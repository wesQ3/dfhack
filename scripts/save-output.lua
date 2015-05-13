-- saves the output of a command to a file
-- By PeridexisErrant

local help = [[
Usage:
    save-output filename.txt command [args]

Saves the output of a comman to a file; designed to work for
settings export (eg `workflow list-commands`).
]]

local args = {...}

-- TODO:  check that file does not exist and does end in *.txt
local fname = tostring(args[1])

-- TODO:  get command by concatenating remaining args
local command = "help"

-- TODO: make this cross-platform
-- dfhack.run_script works... but not for plugins, which are the main point.
local run = '"'..dfhack.getDFPath()..'" dfhack-run.exe'

os.execute(run..' '..command..' > '..fname)
