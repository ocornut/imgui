#!/usr/bin/env ruby

$VERBOSE = 2

INDENT = ' ' * 4

puts <<EOT
#include "imgui.h"

#pragma once

EOT

$current_match = nil
$class_name = nil
$pop_body = nil

def close_class
  pop_body = case $current_match[:name]
             when 'Begin' then 'if (IsOpen) ImGui::End();'
             when /^Tree/ then 'if (IsOpen) ImGui::TreePop();'
             when /^PushID$/ then 'ImGui::PopID();'
             else fail "Don't know what pop body to use"
             end

  puts "#{INDENT}~#{$class_name}() { #{pop_body} }"

  if $current_match[:type] == 'bool'
    puts
    puts "#{INDENT}operator bool() { return IsOpen; }"
  end

  puts "};"
end

header_file = File.open("imgui.h")
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

    if !$current_match || $current_match[:name] != m[:name]
      if $current_match
        close_class
        puts
      end

      $current_match = m
      $class_name = case m[:name]
                    when 'Begin' then 'ImWindow'
                    else "Im#{m[:name]}"
                    end

      puts <<EOT
class #{$class_name}
{
public:
EOT
      if m[:type] == 'bool'
        puts "#{INDENT}#{m[:type]} IsOpen;"
        puts
      end
    end

    attrs = m[:attrs].gsub(/IM_FMT(ARGS|LIST)\(\d+\)/) do |a|
      a.sub(/\d+/) { |index| (index.to_i + 1).to_s }
    end

    print "#{INDENT}#{$class_name}(#{m[:args]})#{attrs} { "

    use_varargs = false
    if argnames.last == '...'
      argnames[-1] = 'ap'
      use_varargs = true
      print "va_list ap; va_start(ap, fmt); "
    end

    print "IsOpen = " if m[:type] == 'bool'

    print "ImGui::#{m[:name]}"
    print 'V' if use_varargs
    print "(#{argnames.join(', ')}); "
    print 'va_end(ap); ' if use_varargs

    puts '}'
  end
end

close_class if $current_match

