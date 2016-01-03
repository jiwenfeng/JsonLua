local json = require "json"

local tb = json.Decode("[{\"key1\":\"value1\", \"key2\":\"value2\", \"key3\":{\"asdf\":\"adf\"} ,\"key5\":123}, \"TT\", \"TT2\"]")

function print_r(tb, msg)
	for k, v in pairs(tb) do
		if(type(v) == "table") then
			print_r(v, k)
		else
			print(k, v, msg)
		end
	end
end

-- print_r(tb)
--
local tb1 = {"s", "a", "b"} -- [a, b, c]

local tb2 = {s = 1, "s", "a", [3] = "t", [5] = "c", [4] = "fasd", TT = {a = 1, b = 2, c = 3}} -- {"s":a}

local tb3 = {{1, 2, 3}, {a = 1, b = 2}}

print(json.Encode(tb))
print(json.Encode(tb1))
print(json.Encode(tb2))
print(json.Encode(tb3))
