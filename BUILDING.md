
### How to build nobuzz

Required tools: Visual Studio 2013 Community edition, WIX Toolset 3.9

1. Open `nobuzz.sln` in Visual Studio 2013

2. Go to `Batch Build...`, build all 4 `Release` targets:
	* nobuzz Release Win32
	* nobuzz Release x64
	* SetupHelper Release Win32
	* SetupHelper Release x64

3. Run `build_setup.cmd`

Output installer will be at `Setup\bin\Release\nobuzz_setup.exe`
