ffi=require 'ffi+'
watchdog_counter = ffi.new 'uint32_t [1]'
local watchdog_counter_addr = tonumber(ffi.cast('uint32_t', watchdog_counter))

-- Check interval in seconds
local watchdog_interval = 1/100  -- 10ms + loop time

-- Failure limit in watchdog intervals
local watchdog_limit = 5         -- Roughly 1/20s

local quit_flag = 0
local radio_fail_flag = 1

function watchdog()
   ffi=require 'ffi+'
   taskman = require 'taskman'
   atomic=ffi.load_with_cdefs './atomic.so'
   watchdog_counter = ffi.cast('uint32_t *', watchdog_counter_addr)
   while not taskman.global_flag(quit_flag) do
      if (watchdog_counter[0] < watchdog_limit) then
	 atomic.increment(watchdog_counter)
	 taskman.change_global_flag(radio_fail_flag, false)
      else
	 taskman.change_global_flag(radio_fail_flag, true)
      end
      taskman.sleep(watchdog_interval)
   end
end

taskman = require 'taskman'
atomic=ffi.load_with_cdefs './atomic.so'

futaba = ffi.load_with_cdefs './futaba.so'

-- This needs to be a udev assigned name.
fd = futaba.open_futaba('/dev/ttyUSB0')

-- Find start of SBus frame.
futaba.sync_futaba(fd)

taskman.set_subscriptions{child_task_exits=true}
taskman.create_task{program=watchdog}

sbus_packet = ffi.new 'uint8_t [25]'
while not taskman.global_flag(quit_flag) do
   futaba.read_futaba_packet(fd, sbus_packet);
   -- Warning!!!  This is from observation rather than documentation.
   -- Bits 2 through 7 are clear in normal operation.
   -- Bits 2 and 3 go active when radio goes offline.
   if sbus_packet[23] < 4 then
      atomic.clear(watchdog_counter)
      -- Do we want this write to be conditioned on "radio signal present?"
--      futaba.write_futaba_packet(fd, sbus_packet);
   end
end

taskman.wait_message()
