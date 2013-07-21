# summary

lua-libmemcached is a simple [Lua](http://www.lua.org/) binding for [libmemcached](http://libmemcached.org/).

# help and support

please fill an issue or help it doing a clone and then a pull request.

# license

[BEER-WARE](http://en.wikipedia.org/wiki/Beerware), see source

# prerequisites

+ [libmemcached](http://libmemcached.org/) (of course) 
+ [Lua](http://www.lua.org/) ([Luajit](http://luajit.org/))
+ [gcc](http://gcc.gnu.org/) and [make](http://www.gnu.org/software/make/) ([min-gw32](http://sourceforge.net/projects/mingw/files/MinGW/) on windows)
+ [upx](http://upx.sourceforge.net/) (optional)

# installation

compatible with *NIX systens, supposed to works on windows. You only need to edit some vars on makefile, for basic instalation use:

    # make

if you have upx use this for better output.
  
    # make release
  
for cleanup

    # make clean
  
# basic usage

```lua

local memc = require('lua-libmemcached')

-- constants ...
-- behavior, further reading http://bit.ly/157qEcD
--[[
memc.BEHAVIOR_NO_BLOCK
memc.BEHAVIOR_TCP_NODELAY
memc.BEHAVIOR_HASH
memc.BEHAVIOR_KETAMA
memc.BEHAVIOR_SOCKET_SEND_SIZE
memc.BEHAVIOR_SOCKET_RECV_SIZE
memc.BEHAVIOR_CACHE_LOOKUPS
memc.BEHAVIOR_SUPPORT_CAS
memc.BEHAVIOR_POLL_TIMEOUT
memc.BEHAVIOR_DISTRIBUTION
memc.BEHAVIOR_BUFFER_REQUESTS
memc.BEHAVIOR_USER_DATA
memc.BEHAVIOR_SORT_HOSTS
memc.BEHAVIOR_VERIFY_KEY
memc.BEHAVIOR_CONNECT_TIMEOUT
memc.BEHAVIOR_RETRY_TIMEOUT
memc.BEHAVIOR_KETAMA_WEIGHTED
memc.BEHAVIOR_KETAMA_HASH
memc.BEHAVIOR_BINARY_PROTOCOL
memc.BEHAVIOR_SND_TIMEOUT
memc.BEHAVIOR_RCV_TIMEOUT
memc.BEHAVIOR_SERVER_FAILURE_LIMIT
memc.BEHAVIOR_IO_MSG_WATERMARK
memc.BEHAVIOR_IO_BYTES_WATERMARK
memc.BEHAVIOR_IO_KEY_PREFETCH
memc.BEHAVIOR_HASH_WITH_PREFIX_KEY
memc.BEHAVIOR_NOREPLY
memc.BEHAVIOR_USE_UDP
memc.BEHAVIOR_AUTO_EJECT_HOSTS
memc.BEHAVIOR_NUMBER_OF_REPLICAS
memc.BEHAVIOR_RANDOMIZE_REPLICA_READ
memc.BEHAVIOR_CORK
memc.BEHAVIOR_TCP_KEEPALIVE
memc.BEHAVIOR_TCP_KEEPIDLE
memc.BEHAVIOR_MAX
--]]
-- server distribution
--[[
memc.DISTRIBUTION_MODULA
memc.DISTRIBUTION_CONSISTENT
memc.DISTRIBUTION_CONSISTENT_KETAMA
memc.DISTRIBUTION_RANDOM
memc.DISTRIBUTION_CONSISTENT_KETAMA_SPY
memc.DISTRIBUTION_CONSISTENT_MAX
--]]
-- hash
--[[
memc.HASH_DEFAULT
memc.HASH_MD5
memc.HASH_CRC
memc.HASH_FNV1_64
memc.HASH_FNV1A_64
memc.HASH_FNV1_32
memc.HASH_FNV1A_32
memc.HASH_HSIEH
memc.HASH_MURMUR
memc.HASH_JENKINS
memc.HASH_CUSTOM
memc.HASH_MAX
--]]
-- connection
--[[
memc.CONNECTION_UNKNOWN
memc.CONNECTION_TCP
memc.CONNECTION_UDP
memc.CONNECTION_UNIX_SOCKET
memc.CONNECTION_MAX
--]]

-- new instance with host/port address(es) and behavior(s), all optional, could raise error
local n_memc = memc.new(
                  'localhost:11211' or {10.1.1.66:11211', '10.1.1.69:11211'}, 
                  {memc.BEHAVIOR_TCP_NODELAY, {memc.BEHAVIOR_POLL_TIMEOUT, 10000}, ...}
                )

-- add server(s), true if successful
n_memc:add_server('localhost:11211' or {10.1.1.66:11211', '10.1.1.69:11211'})

-- set behavior(s), true if sucessful, could raise error
n_memc:set_behavior(
    memc.BEHAVIOR_TCP_NODELAY or {memc.BEHAVIOR_TCP_NODELAY, false} or
     {{memc.BEHAVIOR_DISTRIBUTION, memc.DISTRIBUTION_RANDOM}, {memc.BEHAVIOR_HASH, memc.HASH_CRC}}
  )

-- get behavior value, could raise error
local behavior = n_memc:get_behavior(memc.BEHAVIOR_DISTRIBUTION)

-- add a key/value/ttl/compression, returns true if successful
-- * ttl (optional, default 0) and compression (optional, uses lz4, default false)
n_memc:add(key, value, 3600 or 0, false or true)

-- stores a value in given key with ttl/compression, returns true if sucessful
-- * ttl and compression are optional
n_memc:set(key, value, 7200 or 0, false or true)

-- get value, cas token by key, returns false if not successful
local value, cas_token = n_memc:get(key)

-- replace a value/ttl/compression in given key, returns true if successful.
-- same as set, but fails if the key does not exist on the server
n_memc:replace(key, value, 7200 or 0, false or true)

-- compare and swap a key with value/ttl/compression, returns true if successful.
-- * ttl and compression are optional
n_memc:cas(cas_token, key, value, 86400 or 0, false or true)

-- appends data to existing value, returns true if successful
-- warning, appending value to an already compressed value is not possible.
n_memc:append(key, value)

-- prepends data to existing value, returns true if successful
-- warning, prepending value to an already compressed value is not possible.
n_memc:prepend(key, value)

-- delete key or delete with delayed time, returns true if successful
n_memc:delete(key, 60 or 0)

-- increments value of a given key, optional offset (default 1) and ttl (default 0/no change)
-- returns new value of false if not successful
n_memc:incr(key, 10 or 1, 1800 or 0)

-- decrements value of a given key, optional offset (default 1) and ttl (default 0/no change)
-- returns new value of false if not successful
n_memc:decr(key, 10 or 1, 1800 or 0)

-- same as set, now with multiple key, value pairs
n_memc:set_multi({key = value, key2 = value2, ...})

-- same as get, noew with multiple keys, returns a table with key, value pairs
n_memc:get_multi({key, key2, ...})

-- same as delete, now with multiple keys, returns true if successful
n_memc:delete_multi({key, key2, ...}, 1800 or 0)

-- same as append, now with multiple key, value pairs
n_memc:append_multi({key = value, key2 = value2, ...})

-- same as prepend, now with multiple key, value pairs
n_memc:prepend_multi({key = value, key2 = value2, ...})

-- safe key checksum, return false/true and string with message in case of error
local checksum, err_str = n_memc:check_key("klklksasas8989889asasauhauhauha")


``` 

# tests

see test.lua ...

# TODO

+ support luvit module style
+ create a test suite
+ improve makefile

% July 21th, 2013 -03 GMT
