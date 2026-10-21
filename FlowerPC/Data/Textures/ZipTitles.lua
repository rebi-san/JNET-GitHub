-------------------------------------------------------------------------------
-- Notes:  Used with ZipTitles.bat
--
-- Author: John Edwards
-- Date C: 1-20-2009
-------------------------------------------------------------------------------

for tex in io.lines( "Titles.txt" ) do
	local tempTex = "__Temp_"..tex
	local zipTex = tex..".gz"
	local toolsDir = "..\\..\\..\\Tools"
	local textureTool = toolsDir.."\\nv\\CompressTexture.bat"
	local zipTool = toolsDir.."\\7z\\7za.exe"
	
	print( "del "..tempTex.." 2> nul > nul" )
	print( "call "..textureTool.." "..toolsDir.." "..tex.." "..tempTex.." dxt1" )
	
	print( "del "..zipTex.." 2> nul > nul" )
	print( zipTool.." a -tgzip "..zipTex.." "..tempTex.." -mx7" )
	
	print( "del "..tempTex )
	
	print( "" )
end
