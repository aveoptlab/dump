local ffi = require 'ffi+'
local mmap = ffi.load_with_cdefs '/usr/local/lib/lua/5.1/mmaputil.so'

local arg = {...}[1]

local params = ffi.new('size_t[2]', arg.size)

local cbuf = ffi.cast('uint8_t *',
		      mmaputil.allocate_twinmap(params, params+1))
local size = params[0]

local tail, head = 0,0

local function used() = (tail - head + size) % size end
local function avail() = size - 1 - used() end

local state = 'initial'
local data_len

while not taskman.global_flag(QUIT_FLAG) do
   -- Command and output handling here

   -- Read handling here.
   local read_status = file.status(clientfd, true, false, 1/100)

   if read_status then
      local avail = avail()
      -- avail==0 indicates the buffer is undersized for the traffic.
      if avail==0 then
	 -- log overflow failure, and discard stored.
	 tail,head = 0,0
	 state = 'initial'
      end
      
      if avail > 0 then
	 actual = read(clientfd, cbuf+tail, avail)
	 tail = (tail + actual) % size
      end

      local used = used()
      if state == 'initial' then
	 while used >= 1 and cbuf[head] ~= 0xAA do
	    used = used - 1
	    head = (head + 1) % size
	 end
	 if used == 0 then goto continue end
	 if cbuf[head] == 0xAA then state = 'at_start' end
      end
      if state == 'at_start' then
	 if used < 12 then goto continue end
	 if crc16(cbuf+head, 12) != 0 then
	    head = (head + 1) % size
	    state = 'initial'
	    goto continue
	 end
	 state == 'read_data'
	 -- Real code expects bitfield packing to do the little endian
	 -- thing counter to ANSI C which leaves the order unspecified.
	 data_len = cbuf[1] + 256*cbuf[2];
      end
      if state == 'read_data' and used < data_len then
	 goto continue
      end
   end
   ::continue::
end
