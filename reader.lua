local ffi = require 'ffi+'
local mmaputil = ffi.load_with_cdefs '/usr/local/lib/lua/5.1/mmaputil.so'

function make_dji_handle(name, filedescr, maxsize)
   local size = ffi.new 'size_t[2]'
   local where = size + 1
   size[0] = maxsize
   return {
      name = name
      file = filedescr
      size = size[0]
      where = where[0]
      inbuf = mmaputil.allocate_twinmap(size, where)
      head = 0
      tail = 0
   }
end

-- Assume we have succeeded a select() already.
function read_dji_packet(djidescr)
   local used = (djidescr.tail - djidescr.head + djidescr.size) % size
   local avail = djidescr.size - 1 - used
   assert(avail > 0, "Insufficient buffer space for "..djidescr.name)

   djidescr.file:read(avail)
end
