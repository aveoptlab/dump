local ffi = require 'ffi+'
local mmap = ffi.load_with_cdefs '/usr/local/lib/lua/5.1/mmaputil.so'
taskman = require 'taskman'

local read_queue_size = 2^16

local params = ffi.new('size_t[2]', read_queue_size)

local cbuf = ffi.cast('uint8_t *',
		      mmaputil.allocate_twinmap(params, params+1))
local size = params[0]

local tail, head = 0,0

local function used() = (tail - head + size) % size end
local function avail() = size - 1 - used() end

local state = 'initial'
local data_len

-- dji_fd is assumed to be non-blocking.

while not taskman.global_flag(quit_flag) do
   -- Command and output handling here

   if state == 'packet_complete' then
      -- TODO: Handle complete packet here.
      -- Advance queue head and reset FSM to beginning.
      head = (head + data_len) % size;
      state = 'initial'
   end

   -- Read handling here.
   local read_status = file.status(dji_fd, false, 1/100)

   if read_status then
      local avail = avail()
      -- avail==0 indicates the buffer is undersized for the traffic.
      if avail==0 then
	 -- log overflow failure, and discard stored.
	 tail,head = 0,0
	 state = 'initial'
      end
      
      if avail > 0 then
	 actual = read(dji_fd, cbuf+tail, avail)
	 -- Should error check here.
	 tail = (tail + actual) % size
      end
   end

   -- Try to handle data in input queue if possible.
   local used = used()
   if used > 0 then
      if state == 'initial' then
	 while used >= 1 and cbuf[head] ~= 0xAA do
	    used = used - 1
	    head = (head + 1) % size
	 end
	 -- There was no start of frame.
	 if used == 0 then goto continue end
	 -- Start of frame was found, so advance FSM.
	 state = 'at_start'
      end
      if state == 'at_start' then
	 -- Unless there's enough to compute the first checksum,
	 -- go on with non-read processing until there's more to read.
	 if used < 12 then goto continue end
	 -- If checksum doesn't match, assume the SOF byte was in fact
	 -- not an SOF at all, so look for the next one.
	 if crc16(cbuf+head, 12) != 0 then
	    head = (head + 1) % size
	    state = 'initial'
	    goto continue
	 end
	 state == 'read_data'
	 -- DJI code expects bitfield packing to do the little endian
	 -- thing counter to ANSI C which leaves the order unspecified.
	 data_len = cbuf[1] + 256*cbuf[2];
      end
      if state == 'read_data' used >= data_len then
	 if crc32(cbuf+head, data_len) == 0 then
	    state = 'packet_complete'
	 else
	    -- Packet is corrupt.  Discard, and look for the next SOF.
	    head = (head + data_len) % size;
	    state = 'initial'
      end
   end
   ::continue::
end
