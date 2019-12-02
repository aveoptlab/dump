ffi = require 'ffi'
ffi.cdef [[
uint16_t calculate_crc16(const uint8_t *data, size_t len);
uint32_t calculate_crc32(const uint8_t *data, size_t len);
]]

crc=ffi.load './djicrc.so'

function c16(str)
   return crc.calculate_crc16(str, #str)
end

function c32(str)
   return crc.calculate_crc32(str, #str)
end

teststrings = {
   'This is a test string',
   (function()
	 local a=''
	 for i=0,255 do a = a..string.char(1) end
	 return a
   end)(),
   ''
}

results16 = {'\140\141', '\242\199', '\163\058'}

for i=1,#teststrings do
   assert(c16(teststrings[i]..results16[i]) == 0, ("CRC16 "..i..' FAILED'))
end

results32 = { '\110\087\000\089', '\035\011\063\172', '\163\058\000\000' }

for i=1,#teststrings do
   assert(c32(teststrings[i]..results32[i]) == 0, ("CRC32 "..i..' FAILED'))
end
