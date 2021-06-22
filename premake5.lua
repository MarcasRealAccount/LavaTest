workspace("LavaTest")
	configurations({ "Debug", "Release", "Dist" })
	platforms({ "x64" })
	language("C++")
	cppdialect("C++20")
	rtti("Off")
	exceptionhandling("On")
	flags("MultiProcessorCompile")
	defines({ "LAVA_SYSTEM_%{cfg.system}", "LAVA_TOOLSET_%{cfg.toolset}" })
	
	filter("configurations:Debug")
		defines({ "_DEBUG", "NRELEASE" })
		optimize("Off")
		symbols("On")
	
	filter("configurations:Release")
		defines({ "_RELEASE", "NDEBUG" })
		optimize("Full")
		symbols("On")
	
	filter("configurations:Dist")
		defines({ "_RELEASE", "NDEBUG" })
		optimize("Full")
		symbols("Off")
	
	filter("system:windows")
		toolset("msc")
		defines({ "NOMINMAX", "_CRT_SECURE_NO_WARNINGS" })
	
	filter({ "toolset:gcc", "system:not windows" })
		buildoptions({ "-maccumulate-outgoing-args" })
	
	filter({})
	
	startproject("LavaTest")
	project("Lava")
		kind("ConsoleApp")
		location("Lava")
		targetdir("%{wks.location}/Bin/%{cfg.system}-%{cfg.platform}-%{cfg.buildcfg}")
		objdir("%{wks.location}/BinInt/%{cfg.system}-%{cfg.platform}-%{cfg.buildcfg}/Lava")
		debugdir("%{wks.location}/Run")
		
		files({ "%{prj.location}/**" })
		removefiles({ "**.vcxproj", "**.vcxproj.*", "**/Makefile", "**.make" })
	
	project("LavaCompiler")
		kind("ConsoleApp")
		location("LavaCompiler")
		targetdir("%{wks.location}/Bin/%{cfg.system}-%{cfg.platform}-%{cfg.buildcfg}")
		objdir("%{wks.location}/BinInt/%{cfg.system}-%{cfg.platform}-%{cfg.buildcfg}/LavaCompiler")
		debugdir("%{wks.location}/Run")
		
		files({ "%{prj.location}/**" })
		removefiles({ "**.vcxproj", "**.vcxproj.*", "**/Makefile", "**.make" })
	
	if _ACTION == "vs2019" then
	project("Run")
		kind("None")
		location("Run")
		
		files({ "%{prj.location}/**" })
		removefiles({ "**.vcxproj", "**.vcproj.*", "**/Makefile", "**.make" })
	end