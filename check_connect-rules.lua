check_connect_sensor = "check-connect-status"

CheckConnectStatus = {}
function CheckConnectStatus:new(o)
   o = o or {}
   setmetatable(o, self)
   self.__index = self
   self.current_value = 0
   return o
end

function CheckConnectStatus:report(value)

   print("connect status report value: " .. value)

   -- If value isn't what we had AND its 1 that means
   -- a connection was re-established
   if value ~= self.current_value then
      if value > 0.5 then 
         print("back online")
         monolith_trigger_alert(0, "Back online")
      end
   end

   self.current_value = value
end

-- Helper function
local function string_is_empty(s)
   return s == nil or s == ''
end

-- Create the monitor objects
local connection_status = CheckConnectStatus:new()

-- Called by monolith
function accept_reading_v1_from_monolith(timestamp, node_id, sensor_id, value)

   if string_is_empty(sensor_id) then
      print("Received an empty sensor id?")
      return
   end

   if sensor_id == check_connect_sensor then
      connection_status:report(value)
      return
   end
end