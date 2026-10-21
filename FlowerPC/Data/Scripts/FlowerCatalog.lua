-------------------------------------------------------------------------------
-- Notes:  Functions for hot-loading flower mesh and type data in game
--
-- Author: Martin Middleton
-- Date C: 6-11-2007
-------------------------------------------------------------------------------

-- flowerTypeIndex is intentionally global, as other systems use it.
flowerTypeIndex = {}


local meshTables

-- MB: I reorganized the startup order and found these missing
--     TODO: consolidate all these into a separate lua file and load it
local function VectorAdd( vec0, vec1 )
	local ret = { vec0[1] + vec1[1], vec0[2] + vec1[2], vec0[3] + vec1[3] }
	return ret
end
local function Normalize( vec )
	local weight = math.sqrt (vec[1]*vec[1]+vec[2]*vec[2]+vec[3]*vec[3])
	local ret = { 0, 0, 0 }
	if weight>0 then
		ret = {vec[1]/weight,vec[2]/weight,vec[3]/weight}
	end
	return ret
end

local function InterpretRawMeshData( meshes )
	local interpMeshes = {}
	for i,mesh in ipairs( meshes ) do
		-- The first value is the name, the last is the index list, and the
		-- middle are all frames.
		local meshName = mesh[1]
		local idxs = mesh[ #mesh ]
		local rawFrames = {}
		for i=2,#mesh-1 do rawFrames[i-1] = mesh[i] end

		local frames = {}
		for i,rawFrame in ipairs( rawFrames ) do
			local frameName = rawFrame[1]

			local frame = {}
			for i=2,#rawFrame do
				local v = rawFrame[i]
				table.insert(
						frame,
						{ pos = { v[1], v[2], v[3] }
						, norm = { v[4], v[5], v[6] }
						, uv = { v[7], v[8] }
						}
					)
			end

			frames[ frameName ] = frame
		end
		table.sort( frames )
		interpMeshes[ meshName ] = { frames=frames, idxs=idxs }
	end

	return interpMeshes
end

local function FromOrigin( v )
	return math.sqrt( v[1]*v[1] + v[2]*v[2] + v[3]*v[3] )
end

-- (a -> b) -> [a] -> [b]
-- maps a function of a single value onto every value of a list
local function Map( f, list )
	local newList = {}
	for key,val in pairs( list ) do
		newList[key] = f( val )
	end
	return newList
end  


-- (a -> b -> c) -> [a] -> [b] -> [c]
-- make a list with a function that takes two values and two lists of values
local function ZipWith( f, list0, list1 )
	local newList = {}
	for key,val in ipairs( list0 ) do
		newList[key] = f( list0[key], list1[key] )
	end
	return newList
end

local function CurryLerp( alpha )
	local function lerp( x0, x1 ) return x0 + ( x1 - x0 ) * alpha end
	return lerp
end

local meshesAdded = {}

local meshIndex = {}

-- transfers data from lua table format to the meshCatalog data structure
local function LoadMeshCatalog( meshTables, meshCatalog )
	for meshName,meshData in pairs( meshTables ) do
		
		-- get data from the mesh
		local idxs = meshData.idxs
		firstKey,firstFrameVerts = next(meshData.frames)
		local vertCount = #firstFrameVerts
		local idxCount = #idxs
		local frameCount = 0
		for frameName,frameEntry in pairs( meshData.frames ) do frameCount = frameCount + 1 end
		
		-- reserve space in the catalog for the data
		local mesh = meshCatalog:AddMesh( vertCount, frameCount, idxCount )
		meshIndex[ meshName ] = mesh
		
		-- make an ordered list of frames in the mesh
		local orderedFrames = {}
		for frameName,frameEntry in pairs( meshData.frames ) do 
			table.insert( orderedFrames, frameName )
		end
		table.sort( orderedFrames )
		
		-- add the verts
		local vertNum = 0
		for frameIndex,frameName in ipairs( orderedFrames ) do
			local verts = meshData.frames[ frameName ]
			for i=0,vertCount-1 do
				local vert = verts[1 + i]
				local pos = vert.pos
				local uv = vert.uv
				local norm = vert.norm
				local meshVert = meshCatalog:GetVertInMesh( mesh, vertNum )
				meshVert:SetPos( pos[1], pos[2], pos[3] )
				meshVert:SetUV( uv[1], uv[2] )
				meshVert:SetNorm( norm[1], norm[2], norm[3] )
				vertNum = vertNum + 1
			end
		end
		
		-- add the idxs
		for i=0,idxCount-1 do
			meshCatalog:SetIdxInMesh( mesh, i, idxs[i + 1] )
		end
		
		CreateLodMesh( meshName, meshData, orderedFrames, meshCatalog )
		
	end
	
end

function AddQuadVerts( mesh, meshCatalog, box, firstVertNum )
	
	local norm = box.norm
	
	--[[Figure out which face we are using
	local dirx = math.abs( norm[ 1 ] )
	local diry = math.abs( norm[ 2 ] )
	local dirz = math.abs( norm[ 3 ] )
	local face = "z"
	if dirx > dirz and dirx > diry then 
		face = "x"
	else 
		if diry > dirz and diry > dirx then 
			face = "y" 
		end
	end]]
	
	local quad = 
		{ { box.x[ 1 ], box.y[ 1 ], box.z[ 2 ] }
		, { box.x[ 2 ], box.y[ 1 ], box.z[ 2 ] }
		, { box.x[ 2 ], box.y[ 2 ], box.z[ 1 ] }
		, { box.x[ 1 ], box.y[ 2 ], box.z[ 1 ] } }
	
	local meshVert0 = meshCatalog:GetVertInMesh( mesh, 0 + firstVertNum )
	meshVert0:SetPos( quad[ 1 ][ 1 ], quad[ 1 ][ 2 ], quad[ 1 ][ 3 ] )
	meshVert0:SetNorm( norm[ 1 ], norm[ 2 ], norm[ 3 ] )
	meshVert0:SetUV( 0.0, 0.0 )
	local meshVert1 = meshCatalog:GetVertInMesh( mesh, 1 + firstVertNum )
	meshVert1:SetPos( quad[ 2 ][ 1 ], quad[ 2 ][ 2 ], quad[ 2 ][ 3 ] )
	meshVert1:SetNorm( norm[ 1 ], norm[ 2 ], norm[ 3 ] )
	meshVert1:SetUV( 1.0, 0.0 )
	local meshVert2 = meshCatalog:GetVertInMesh( mesh, 2 + firstVertNum )
	meshVert2:SetPos( quad[ 3 ][ 1 ], quad[ 3 ][ 2 ], quad[ 3 ][ 3 ] )
	meshVert2:SetNorm( norm[ 1 ], norm[ 2 ], norm[ 3 ] )
	meshVert2:SetUV( 1.0, 1.0 )
	local meshVert3 = meshCatalog:GetVertInMesh( mesh, 3 + firstVertNum )
	meshVert3:SetPos( quad[ 4 ][ 1 ], quad[ 4 ][ 2 ], quad[ 4 ][ 3 ] )
	meshVert3:SetNorm( norm[ 1 ], norm[ 2 ], norm[ 3 ] )
	meshVert3:SetUV( 0.0, 1.0 )
end

function CreateLodMesh( meshName, meshData, orderedFrames, meshCatalog )

	local boxFrames = {}

	-- Figure out the bounding box
	firstKey,firstFrameVerts = next(meshData.frames)
	local vertCount = #firstFrameVerts
	for frameIndex,frameName in ipairs( orderedFrames ) do
		local bbox = { x = { 99999, -99999 }, y = { 99999, -99999 }, z = { 99999, -99999 }, norm = { 0.0, 0.0, -1.0 } }
		local verts = meshData.frames[ frameName ]
		for i=0,vertCount-1 do
			local vert = verts[1 + i]
			local pos = vert.pos
			if pos[ 1 ] < bbox.x[ 1 ] then bbox.x[ 1 ] = pos[ 1 ] end
			if pos[ 1 ] > bbox.x[ 2 ] then bbox.x[ 2 ] = pos[ 1 ] end
			if pos[ 2 ] < bbox.y[ 1 ] then bbox.y[ 1 ] = pos[ 2 ] end
			if pos[ 2 ] > bbox.y[ 2 ] then bbox.y[ 2 ] = pos[ 2 ] end
			if pos[ 3 ] < bbox.z[ 1 ] then bbox.z[ 1 ] = pos[ 3 ] end
			if pos[ 3 ] > bbox.z[ 2 ] then bbox.z[ 2 ] = pos[ 3 ] end
			bbox.norm = VectorAdd( bbox.norm, vert.norm )
		end
		bbox.norm = Normalize( bbox.norm )
		boxFrames[ frameIndex ] = bbox
	end
	
	-- Create the mesh
	local mesh = meshCatalog:AddMesh( 4, 2, 6 )
	meshIndex[ meshName.."QuadLow" ] = mesh
	

	AddQuadVerts( mesh, meshCatalog, boxFrames[ 1 ], 0 )
	
	AddQuadVerts( mesh, meshCatalog, boxFrames[ #orderedFrames ], 4 )
	
	-- Add idxs
	meshCatalog:SetIdxInMesh( mesh, 0, 0 )
	meshCatalog:SetIdxInMesh( mesh, 1, 1 )
	meshCatalog:SetIdxInMesh( mesh, 2, 2 )
	meshCatalog:SetIdxInMesh( mesh, 3, 0 )
	meshCatalog:SetIdxInMesh( mesh, 4, 2 )
	meshCatalog:SetIdxInMesh( mesh, 5, 3 )

end

function LoadFlowerCatalog( flowerData, flowerCatalog )

	local bUseHigh = false
	if TARGET=="PS4" then
		bUseHigh = true
	end

	-- for each flower in the lua flower catalog
	for i,flowerType in ipairs( flowerData ) do
		
		--add a flower in the catalog
		local flower = flowerCatalog:AddFlower()
		flowerTypeIndex[ flowerType.flowerName ] = i - 1
		
		--for each petal layer
		for i,layerData in ipairs( flowerType.petalLayers ) do
			
			--add some layer stuff
			local layer = flowerCatalog:AddLayerToFlower( flower )
			
			if( layerData.rot ) then
				local rot = layerData.rot
				rot = Map( math.rad, rot )
				layer:SetRot( rot[1], rot[2], rot[3] )
			else
				local rotBloom = layerData.rotBloom
				rotBloom = Map( math.rad, rotBloom )
				layer:SetRotBloom( rotBloom[1], rotBloom[2], rotBloom[3] )
				local rotUnBloom = layerData.rotUnBloom
				rotUnBloom = Map( math.rad, rotUnBloom )
				layer:SetRotUnBloom( rotUnBloom[1], rotUnBloom[2], rotUnBloom[3] )
			end
			
			-- MB: numPetalsHigh member allows PS4 to add petals
			if layerData.numPetalsHigh and bUseHigh then
				layer:numPetals( layerData.numPetalsHigh )
			else
				layer:numPetals( layerData.numPetals )
			end
			
			if type( layerData.scale ) == "table" then layer:SetScale( layerData.scale[ 1 ], layerData.scale[ 2 ], layerData.scale[ 3 ] )
			else layer:SetScale( layerData.scale, layerData.scale, layerData.scale ) end
			if layerData.scaleUnBloom then 
				if type( layerData.scaleUnBloom ) == "table" then layer:SetScaleUnBloom( layerData.scaleUnBloom[ 1 ], layerData.scaleUnBloom[ 2 ], layerData.scaleUnBloom[ 3 ] )
				else layer:SetScaleUnBloom( layerData.scaleUnBloom, layerData.scaleUnBloom, layerData.scaleUnBloom ) end
			end
			if layerData.offset then layer:SetOffset( layerData.offset[ 1 ], layerData.offset[ 2 ], layerData.offset[ 3 ] )
			else layer:SetOffset( 0, 0, 0 ) end
			if layerData.offsetUnBloom then layer:SetOffsetUnBloom( layerData.offsetUnBloom[ 1 ], layerData.offsetUnBloom[ 2 ], layerData.offsetUnBloom[ 3 ] ) end
			
			local stemLerpLow = 1
			local stemLerpHigh = 1
			if( layerData.stemLerp ) then 
				stemLerpLow = layerData.stemLerp[ 1 ] 
				stemLerpHigh = layerData.stemLerp[ 2 ] 
			end
			layer:SetStemLerp( stemLerpLow, stemLerpHigh )
			
			if layerData.randMultBloom then layer:randMultBloom( layerData.randMultBloom ) else layer:randMultBloom( 1.0 ) end
			if layerData.randMultUnBloom then layer:randMultUnBloom( layerData.randMultUnBloom ) else layer:randMultUnBloom( 1.0 ) end
			
			--add some petal stuff	
			local petal = flowerCatalog:GetPetalFromLayer( layer )
			
			local meshNum = meshIndex[ layerData.meshName ]
			petal:SetMeshNum( meshNum )
			
			-- for low LOD
			petal:meshNumSmall( meshIndex[ layerData.meshName.."QuadLow" ] )
			--petal:meshNumSmall( meshIndex[ "QuadLow" ] )
			
			petal:SetColorBase( layerData.color0[1], layerData.color0[2], layerData.color0[3] )
			petal:SetColorTip( layerData.color1[1], layerData.color1[2], layerData.color1[3] )
			
			if layerData.color0UnBloom then petal:SetColorBaseUnBloom( layerData.color0UnBloom[1], layerData.color0UnBloom[2], layerData.color0UnBloom[3] ) end
			if layerData.color1UnBloom then petal:SetColorTipUnBloom( layerData.color1UnBloom[1], layerData.color1UnBloom[2], layerData.color1UnBloom[3] ) end

			local petalTex = layerData.texture
			if petalTex == nil then petalTex = "Default" end
			local atlasGridU = petalCatalog[ petalTex ][1]
			local atlasGridV = petalCatalog[ petalTex ][2]
			petal:SetAtlasPos( atlasGridU, atlasGridV )
				
		end
		
		-- Set stem texture
		local stemTex = flowerType.stemTexture
		if stemTex == nil then stemTex = "StemDefault" end
		local atlasGridU = petalCatalog[ stemTex ][1]
		local atlasGridV = petalCatalog[ stemTex ][2]
		flowerCatalog:SetStemAtlasPos( flower, atlasGridU, atlasGridV )
		
		-- Set height mult
		if flowerType.heightMult then flowerCatalog:SetHeightMult( flower, flowerType.heightMult )
		else flowerCatalog:SetHeightMult( flower, 1 ) end
		
		if flowerType.outerLayer then flowerCatalog:SetOuterLayers( flower, flowerType.outerLayer[ 1 ], flowerType.outerLayer[ 2 ] )
		else flowerCatalog:SetOuterLayers( flower, 0, 0 ) end
		
		-- Set stem color
		flowerCatalog:SetStemBaseColor( flower, flowerType.stemBaseColor[ 1 ], flowerType.stemBaseColor[ 2 ], flowerType.stemBaseColor[ 3 ] )
		flowerCatalog:SetStemTipColor( flower, flowerType.stemTipColor[ 1 ], flowerType.stemTipColor[ 2 ], flowerType.stemTipColor[ 3 ] )
		
		-- Set glow color
		flowerCatalog:SetGlowBottomColor( flower, flowerType.glowBottomColor[ 1 ], flowerType.glowBottomColor[ 2 ], flowerType.glowBottomColor[ 3 ], flowerType.glowBottomColor[ 4 ] )
		flowerCatalog:SetGlowTopColor( flower, flowerType.glowTopColor[ 1 ], flowerType.glowTopColor[ 2 ], flowerType.glowTopColor[ 3 ], flowerType.glowTopColor[ 4 ] )
		
		-- Set droop		
		if flowerType.sadness then flowerCatalog:SetSadness( flower, flowerType.sadness )
		else flowerCatalog:SetSadness( flower, 0.0 ) end
	end
end


-- Called when the flower catalog is reloaded
function LoadFlowerData( meshCatalog, flowerCatalog, flowerBarn )
	meshIndex = {}
	meshesAdded = {}
	meshTables = InterpretRawMeshData( flowerMeshes )
	LoadMeshCatalog( meshTables, meshCatalog )
	LoadFlowerCatalog( flowerData, flowerCatalog )
		
	local atlasScaleWidth = petalCatalog.gridWidth / petalCatalog.atlasWidth
	local atlasScaleHeight = petalCatalog.gridWidth / petalCatalog.atlasHeight
	flowerCatalog:SetAtlasScale( atlasScaleWidth, atlasScaleHeight )
	
	flowerBarn:SetSecretFlowerType( flowerTypeIndex[ "Special" ] )
end

-- Reload flower positions
function LoadFlowerMap( flowerBarn, terrainData )

	flowerBarn:SetHeightRange( flowerHeight[ 1 ], flowerHeight[ 2 ] )		

	-- Add flowers to the flowerBarn	
	for i,flowerEntry in ipairs( FlowerInstances ) do
		local typeName = flowerEntry.FlowerType
		local typeNum = flowerTypeIndex[ typeName ]
		local pos = flowerEntry.Position
		local isEnabled = flowerEntry.Enabled
		
		local flowerPtr = nil
		if flowerEntry.Float then
			flowerPtr = flowerBarn:AddNewLargeFlowerXYZ( typeNum, pos[ 1 ], pos[ 2 ] , pos[ 3 ] )
		else
			flowerPtr = flowerBarn:AddNewLargeFlowerXZ( typeNum, pos[ 1 ], pos[ 3 ], terrainData )
		end
		
		if isEnabled == nil then flowerPtr:SetActive( true ) else flowerPtr:SetActive( isEnabled ) end
		if isEnabled == false then flowerPtr:SetAge( 0.0 ) end
		
		if flowerEntry.GlowEnabled then flowerPtr:SetGlowEnabled( true ) else flowerPtr:SetGlowEnabled( false ) end
	
		--load the flowers into the nametable so we can clump them
		Names[flowerEntry.ObjectName] = flowerPtr
	end

	FlowerInstances = {}

end

function FindFlowerType( typeName )
    local typeNum = flowerTypeIndex[ typeName ]
    if ( typeNum == nil ) then 
		print("FindFlowerType: Bad typeName - "..typeName)
	end
	return typeNum
end 

