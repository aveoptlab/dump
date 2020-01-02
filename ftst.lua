function display()
   f=require 'ffi'
   f.cdef 'void write(int, const char *, size_t);'
   t = require 'taskman'
   while not t.global_flag(0) do
      local msg = 'Futaba state: '..tostring(t.global_flag(1))..'\n'
      f.C.write(2, msg, #msg)
      t.sleep(1/5)
   end
end

t=require 'taskman'
t.set_subscriptions{child_task_exits=true}
t.create_task{program=':futaba-loop.lua', show_errors=true}
t.create_task{program=display}
lua_repl()
t.change_global_flag(0, true)
t.wait_message()
t.wait_message()
t.shutdown()
