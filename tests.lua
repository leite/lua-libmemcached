local mem = require 'lua-libmemcached'

local function simple_table_dump(...)
  local tb, arg = ...
  if arg then print(arg) end
  for k,v in pairs(tb) do
    print(string.format('  [%s] = %s', tostring(k), tostring(v)))
  end
end

local inst = mem.new("localhost", 11211, {no_block=true, use_binary=true}, nil)

inst:add_server({"127.0.0.1", 11211})

print('>>\n>>')

print('  copyright            = ' .. mem._COPYRIGHT)
print('  description          = ' .. mem._DESCRIPTION)
print('  version              = ' .. mem._VERSION)
print('  libmemcached version = ' .. mem._LIBVERSION)

print('>>\n>>')

print(inst:set_behavior({use_binary=true}))

local behaviors = inst:get_behavior({"use_udp", "no_block", "use_binary"})
simple_table_dump(behaviors, " :: behaviors :: ")

print(inst:set("foo", "xxxxyyyzzz zzzz www"))
print(inst:set({mee=11, moo="okey dokey"}))

local rtn = inst:get({"moo", "foo"})
print(rtn)
if type(rtn)=="table" then
  simple_table_dump(rtn)
else
  print(rtn)
end

local ax, bx = inst:get("mee")

print(ax)
print(bx)

inst = nil
collectgarbage()

print('>>\n>>')
print('this is the end, my only friend, the end ...')
