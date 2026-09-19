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
                   'IsOpen' if m[:type] == %w{bool}
                 end

    puts <<EOT
#{INDENT}struct #{@class_name}
#{INDENT}{
EOT
    if @state_var
      puts "#{INDENT * 2}#{m[:type].join('')} #{@state_var};"
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
          when /^Begin(.*)/
            body = "ImGui::End#{$1}();"
            body = "if (#{@state_var}) " + body if @state_var
            body
          when /^TreeNode/ then "if (#{@state_var}) ImGui::TreePop();"
          when 'TreePush' then 'ImGui::TreePop();'
          when /^Push(.*)/ then "ImGui::Pop#{$1}();"
          else fail "Don't know what pop body to use for #{@name}"
          end
    puts " }"

    if @state_var
      puts
      puts "#{INDENT * 2}explicit operator bool() const { return #{@state_var}; }"
    end

    puts
    puts "#{INDENT * 2}IMGUI_DELETE_MOVE_COPY(#{@class_name});"
    puts "#{INDENT}};"
  end
end

def is_space(token)
  token =~ /^\s+/
end

def first_non_space(tokens)
  tokens.find { |token| !is_space(token) }
end

def skip_spaces(tokens)
  tokens.shift while is_space(tokens.first)
end

def parse_until(tokens, stop_token)
  parsed = []

  while first_non_space(tokens) != stop_token
    token = tokens.shift
    fail "End of tokens while looking for #{stop_token}" unless token
    parsed << token
  end
  skip_spaces(tokens)
  tokens.shift

  parsed
end

def chop_space(tokens)
  tokens.pop if is_space(tokens.last)
end

def parse_method(line)
  m = {}

  tokens = line.scan(/\w+|\s+|\.\.\.|\/\/.*$|./)
  skip_spaces(tokens)

  return nil unless tokens.shift == 'IMGUI_API'
  skip_spaces(tokens)

  type_and_name = parse_until(tokens, '(')
  m[:name] = type_and_name.pop
  chop_space(type_and_name)
  m[:type] = type_and_name

  return nil unless m[:name].match(/^(Begin|Push|TreeNode)/)

  m[:args] = [[]]
  m[:argnames] = []
  paren_level = 0
  in_default = false
  loop do
    token = tokens.shift
    fail "End of tokens while parsing argument list" unless token

    case token
    when '('
      paren_level+= 1
    when ')'
      break if paren_level == 0
      paren_level-= 1
    when ','
      if paren_level == 0
        m[:args] << []
        in_default = false
        next
      end
    end

    if paren_level == 0 && !in_default
      next_non_space = first_non_space(tokens)

      if next_non_space == ')' || next_non_space == ',' || next_non_space == '='
        m[:argnames] << token
        in_default = (next_non_space == '=')
      end
    end

    m[:args][-1] << token
  end

  m[:attrs] = parse_until(tokens, ';')

  skip_spaces(tokens)
  m[:rest] = tokens

  m
end

current_class = nil

header_file = File.open("../../imgui.h")
header_file.each_line do |line|
  break if line.match(/^}\s*\/\/\s*namespace ImGui$/i)

  m = parse_method(line)
  next unless m

  next unless m[:name].match(/^(Begin|Push|Tree)/)

  $stderr.puts m.inspect if $-d

  # Check for obsolete
  if m[:name] == 'Begin' &&
      (m[:argnames] == %w{name p_open size_on_first_use bg_alpha_override flags} ||
       m[:argnames] == %w{items_count items_height})
    next
  end

  fail "Return value #{m[:type]} is not bool or void" unless [%w{bool}, %w{void}].include?(m[:type])

  if !current_class || current_class.name != m[:name]
    if current_class
      current_class.close
      puts
    end

    current_class = WrapperClass.new(m)
  end

  shift_index = false
  attrs = m[:attrs].map do |attr|
    case attr
    when /^IM_FMT(ARGS|LIST)$/
      shift_index = true
    when /^\d+/
      if shift_index
        attr = (attr.to_i + 1).to_s
        shift_index = false
      end
    end
    attr
  end.join('')

  args = m[:args].map { |argparts| argparts.join('') }.join(',')
  print "#{INDENT * 2}#{current_class.class_name}(#{args})#{attrs} {"

  use_varargs = false
  if m[:argnames].last == '...'
    m[:argnames][-1] = 'ap'
    use_varargs = true
    print " va_list ap; va_start(ap, fmt);"
  end

  if m[:name] =~ /^TreeNodeEx/ && m[:argnames].include?('flags')
    print " IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen));"
  end

  print " #{current_class.state_var} =" if current_class.state_var

  print " ImGui::#{m[:name]}"
  print 'V' if use_varargs
  print "(#{m[:argnames].join(', ')}); "
  print 'va_end(ap); ' if use_varargs

  puts '}'
end

current_class.close if current_class

puts <<EOT

#{INDENT}#undef IMGUI_DELETE_MOVE_COPY

} // namespace ImScoped
EOT
