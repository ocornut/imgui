#!/usr/bin/env ruby

$VERBOSE = 2

INDENT = ' ' * 4

puts <<EOT
#pragma once

#include "imgui.h"

namespace ImScoped
{
#{INDENT}#define IMGUI_DELETE_MOVE_COPY(Base)#{INDENT}                         \\
#{INDENT}#{INDENT}Base(Base&&) = delete;                /* Move not allowed */ \\
#{INDENT}#{INDENT}Base &operator=(Base&&) = delete;     /* "" */               \\
#{INDENT}#{INDENT}Base(const Base&) = delete;           /* Copy not allowed */ \\
#{INDENT}#{INDENT}Base& operator=(const Base&) = delete /* "" */

EOT

class WrapperClass
  attr_reader :name, :class_name, :state_var

  def initialize(m)
    @name = m[:name]

    @class_name = case @name
                  when 'Begin'
                    'Window'
                  when /^Begin(.*)$/
                    $1
                  when /^Push(.*)$/
                    $1
                  else
                    m[:name]
                  end

    @state_var = case @name
                 when 'Begin', 'BeginChild'
                   'IsContentVisible'
                 else
                   'IsOpen' if m[:type] == 'bool'
                 end

    puts <<EOT
#{INDENT}struct #{@class_name}
#{INDENT}{
EOT
    if @state_var
      puts "#{INDENT * 2}#{m[:type]} #{@state_var};"
      puts
    end
  end

  def close
    print "#{INDENT * 2}~#{@class_name}() { "
    print case @name
          when 'Begin' then 'ImGui::End();'
          when 'BeginChild' then 'ImGui::EndChild();'
          when 'BeginChildFrame' then 'ImGui::EndChildFrame();'
          when /^BeginPopup/ then "if (#{@state_var}) ImGui::EndPopup();"
          when /^Begin(.*)/ then "if (#{@state_var}) ImGui::End#{$1}();"
          when /^TreeNode/ then "if (#{@state_var}) ImGui::TreePop();"
          when 'TreePush' then 'ImGui::TreePop();'
          when /^Push(.*)/ then "ImGui::Pop#{$1}();"
          else fail "Don't know what pop body to use for #{@name}"
          end
    puts " }"

    if @state_var
      puts
      puts "#{INDENT * 2}operator bool() { return #{@state_var}; }"
    end

    puts
    puts "#{INDENT * 2}IMGUI_DELETE_MOVE_COPY(#{@class_name});"
    puts "#{INDENT}};"
  end
end

current_class = nil

header_file = File.open("../../imgui.h")
header_file.each_line do |line|
  break if line.match(/^}\s*\/\/\s*namespace ImGui$/i)

  line.match(/^\s*IMGUI_API\s+(?<type>[\w\s]+\w)\s+(?<name>\w*)\((?<args>[^)]+)\)(?<attrs>[^;]*);(?<rest>.*)$/) do |m|
    next unless m[:name].match(/^(Begin|Push|Tree)/)

    argnames = m[:args].split(/,\s*/).map do |arg|
      arg = arg.split(/\s*=\s*/, 2).first
      arg.split(/[ \*]/).last
    end

    # Check for obsolete
    if m[:name] == 'Begin' &&
        (argnames == %w{name p_open size_on_first_use bg_alpha_override flags} ||
         argnames == %w{items_count items_height})
      next
    end

    $stderr.puts m.inspect if $-d

    fail "Return value #{m[:type]} is not bool or void" unless %w{bool void}.include?(m[:type])

    if !current_class || current_class.name != m[:name]
      if current_class
        current_class.close
        puts
      end

      current_class = WrapperClass.new(m)
    end

    attrs = m[:attrs].gsub(/IM_FMT(ARGS|LIST)\(\d+\)/) do |a|
      a.sub(/\d+/) { |index| (index.to_i + 1).to_s }
    end

    print "#{INDENT * 2}#{current_class.class_name}(#{m[:args]})#{attrs} { "

    use_varargs = false
    if argnames.last == '...'
      argnames[-1] = 'ap'
      use_varargs = true
      print "va_list ap; va_start(ap, fmt); "
    end

    print "#{current_class.state_var} = " if current_class.state_var

    print "ImGui::#{m[:name]}"
    print 'V' if use_varargs
    print "(#{argnames.join(', ')}); "
    print 'va_end(ap); ' if use_varargs

    puts '}'
  end
end

current_class.close if current_class

puts <<EOT

#{INDENT}#undef IMGUI_DELETE_MOVE_COPY

} // namespace ImScoped
EOT
