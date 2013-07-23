local mem = require 'lua-libmemcached'

local inst = mem.new()

print('>>\n>>')
print(mem)
print('  copyright   = ' .. mem._COPYRIGHT)
print('  description = ' .. mem._DESCRIPTION)
print('  version     = ' .. mem._VERSION)

print('>>\n>>')
print(inst)


inst:set()
print(inst:get())

inst = nil

collectgarbage()

print('>>\n>>')
print('this is the end, my only friend, the end ...')
