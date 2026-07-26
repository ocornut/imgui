# This file implements synthetic children providers and summaries for various Dear ImGui types for LLDB.
# LLDB is used by Xcode, Android Studio, and may be used from VS Code, C++Builder, CLion, Eclipse etc.

#
# Useful links/documentation related to the feature:
# - https://lldb.llvm.org/use/variable.html#summary-strings
# - https://lldb.llvm.org/use/variable.html#synthetic-children
# - https://lldb.llvm.org/python_reference/lldb-module.html
#
# To use it in a debug session:
# > (lldb) command script import <path-to-this-file>
#
# Alternatively you may include the above command in your ~/.lldbinit file to have the formatters
# available in all future sessions

import lldb

class ArraySynthBase(object):
	"""
	Helper baseclass aimed to reduce the boilerplate needed for "array-like" containers
	"""

	def __init__(self, valobj, internal_dict):
		self.valobj = valobj

	def bind_to(self, pointer, size):
		array_p = pointer.GetType().GetPointeeType().GetArrayType(size).GetPointerType()
		self.array = pointer.Cast(array_p).Dereference()

	def update(self):
		self.array = self.valobj

	def num_children(self, max_children):
		return self.array.GetNumChildren(max_children)

	def get_child_index(self, name):
		return self.array.GetIndexOfChildWithName(name)

	def get_child_at_index(self, index):
		return self.array.GetChildAtIndex(index)

	def has_children(self):
		return self.array.MightHaveChildren()

	def get_value(self):
		return self.array

class ImVectorSynth(ArraySynthBase):
	def update(self):
		self.size = self.valobj.GetChildMemberWithName("Size").GetValueAsUnsigned()
		self.capacity = self.valobj.GetChildMemberWithName("Capacity").GetValueAsUnsigned()

		data = self.valobj.GetChildMemberWithName("Data")
		
		self.bind_to(data, self.size)

	def get_summary(self):
		return f"Size={self.size} Capacity={self.capacity}"

class ImSpanSynth(ArraySynthBase):
	def update(self):
		data = self.valobj.GetChildMemberWithName("Data")
		end = self.valobj.GetChildMemberWithName("DataEnd")
		
		element_size = data.GetType().GetPointeeType().GetByteSize()
		array_size = end.GetValueAsUnsigned() - data.GetValueAsUnsigned()

		self.size = int(array_size / element_size)

		self.bind_to(data, self.size)

	def get_summary(self):
		return f"Size={self.size}"

class ImRectSummary(object):
	def __init__(self, valobj, internal_dict):
		self.valobj = valobj

	def update(self):
		pass

	def get_summary(self):
		min = self.valobj.GetChildMemberWithName("Min")
		max = self.valobj.GetChildMemberWithName("Max")

		minX = float(min.GetChildMemberWithName("x").GetValue())
		minY = float(min.GetChildMemberWithName("y").GetValue())

		maxX = float(max.GetChildMemberWithName("x").GetValue())
		maxY = float(max.GetChildMemberWithName("y").GetValue())

		return f"Min=({minX}, {minY}) Max=({maxX}, {maxY}) Size=({maxX - minX}, {maxY - minY})"

def get_active_enum_flags(valobj):
	flag_set = set()

	enum_name = valobj.GetType().GetName() + "_"
	enum_type = valobj.GetTarget().FindFirstType(enum_name)

	if not enum_type.IsValid():
		return flag_set

	enum_members = enum_type.GetEnumMembers()
	value = valobj.GetValueAsUnsigned()

	for i in range(0, enum_members.GetSize()):
		member = enum_members.GetTypeEnumMemberAtIndex(i)

		if value & member.GetValueAsUnsigned():
			flag_set.add(member.GetName().removeprefix(enum_name))

	return flag_set

class ImGuiWindowSummary(object):
	def __init__(self, valobj, internal_dict):
		self.valobj = valobj

	def update(self):
		pass

	def get_summary(self):
		name = self.valobj.GetChildMemberWithName("Name").GetSummary()

		active = self.valobj.GetChildMemberWithName("Active").GetValueAsUnsigned() != 0
		was_active = self.valobj.GetChildMemberWithName("WasActive").GetValueAsUnsigned() != 0
		hidden = self.valobj.GetChildMemberWithName("Hidden") != 0

		flags = get_active_enum_flags(self.valobj.GetChildMemberWithName("Flags"))

		active = 1 if  active or was_active else 0
		child = 1 if "ChildWindow" in flags else 0
		popup = 1 if "Popup" in flags else 0
		hidden = 1 if hidden else 0

		return f"Name {name} Active {active} Child {child} Popup {popup} Hidden {hidden}"


def __lldb_init_module(debugger, internal_dict):
	"""
	This function will be automatically called by LLDB when the module is loaded, here
	we register the various synthetics/summaries we have build before
	"""

	category_name = "imgui"
	category = debugger.GetCategory(category_name)

	# Make sure we don't accidentally keep accumulating languages or override the user's
	# category enablement in Xcode, where lldb-rpc-server loads this file once for eac
	# debugging session
	if not category.IsValid():
		category = debugger.CreateCategory(category_name)
		category.AddLanguage(lldb.eLanguageTypeC_plus_plus)
		category.SetEnabled(True)

	def add_summary(typename, impl):
		summary = None

		if isinstance(impl, str):
			summary = lldb.SBTypeSummary.CreateWithSummaryString(impl)
			summary.SetOptions(lldb.eTypeOptionCascade)
		else:
			# Unfortunately programmatic summary string generation is an entirely different codepath
			# in LLDB. Register a convenient trampoline function which makes it look like it's part
			# of the SyntheticChildrenProvider contract
			summary = lldb.SBTypeSummary.CreateWithScriptCode(f'''
				synth = {impl.__module__}.{impl.__qualname__}(valobj.GetNonSyntheticValue(), internal_dict)
				synth.update()

				return synth.get_summary()
			''')
			summary.SetOptions(lldb.eTypeOptionCascade | lldb.eTypeOptionFrontEndWantsDereference)

		category.AddTypeSummary(lldb.SBTypeNameSpecifier(typename, True), summary)

	def add_synthetic(typename, impl):
		add_summary(typename, impl)

		synthetic = lldb.SBTypeSynthetic.CreateWithClassName(f"{impl.__module__}.{impl.__qualname__}")
		synthetic.SetOptions(lldb.eTypeOptionCascade | lldb.eTypeOptionFrontEndWantsDereference)

		category.AddTypeSynthetic(lldb.SBTypeNameSpecifier(typename, True), synthetic)

	add_synthetic("^ImVector<.+>$", ImVectorSynth)
	add_synthetic("^ImSpan<.+>$", ImSpanSynth)

	add_summary("^ImVec2$", "x=${var.x} y=${var.y}")
	add_summary("^ImVec4$", "x=${var.x} y=${var.y} z=${var.z} w=${var.w}")
	add_summary("^ImRect$", ImRectSummary)
	add_summary("^ImGuiWindow$", ImGuiWindowSummary)
