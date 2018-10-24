#!/usr/bin/env ruby

$VERBOSE = 2

INDENT = ' ' * 4

puts <<EOT
#include "imgui.h"

#pragma once

EOT

class WrapperClass
  attr_reader :name, :class_name, :state_var

  def initialize(m)
    @name = m[:name]

    @state_var = nil
    case m[:name]
    when 'Begin'
      @class_name = 'ImWindow'
      @state_var = 'IsExpanded'
    else
      @class_name = "Im#{m[:name]}"
      @state_var = 'IsOpen' if m[:type] == 'bool'
    end

    puts <<EOT
struct #{@class_name}
{
EOT
    if @state_var
      puts "#{INDENT}#{m[:type]} #{@state_var};"
      puts
    end
  end

  def close
    print "#{INDENT}~#{@class_name}() { "
    print case @name
          when 'Begin' then 'ImGui::End();'
          when /^Tree/ then "if (#{@state_var}) ImGui::TreePop();"
          when /^PushID$/ then 'ImGui::PopID();'
          else fail "Don't know what pop body to use"
          end
    puts " }"

    if @state_var
      puts
      puts "#{INDENT}operator bool() { return #{@state_var}; }"
    end

    puts
    puts "#{INDENT}#{@class_name}(#{@class_name} &&) = delete;"
    puts "#{INDENT}#{@class_name} &operator=(#{@class_name} &&) = delete;"
    puts "#{INDENT}#{@class_name}(const #{@class_name} &) = delete;"
    puts "#{INDENT}#{@class_name} &operator=(#{@class_name} &) = delete;"
    puts '};'
  end
end

current_class = nil

header_file = File.open("../../imgui.h")
header_file.each_line do |line|
  line.match(/^\s*IMGUI_API\s+(?<type>[\w\s]+\w)\s+(?<name>\w*)\((?<args>[^)]+)\)(?<attrs>[^;]*);(?<rest>.*)$/) do |m|
    next unless m[:name].match(/^(Begin|Push|Open|Tree|Column)/)
    next unless m[:name].match(/^TreeNode|PushID|Begin$/)

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

    print "#{INDENT}#{current_class.class_name}(#{m[:args]})#{attrs} { "

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

