-------------------------------------------------------------------------------
-- Notes:  Function for translating Lua sign data to native C++ objects.
--
-- Author: John Edwards
-- Date C: 5-20-2008
-------------------------------------------------------------------------------

function SignInitialize( sign, signInst )
	sign:SetTrans( signInst.Transformation );
		
	--@ADDED By Nick on 10.15.2008
	--Gardener now exports color + alpha (a float)
	local color = signInst.Color
	color[ 4 ] = signInst.Alpha * 255
	sign:m_color( color )
end

function InitializeSignBarn( signBarn, signData, signInstances )
	local credSec, textEnt

--  load the text entries from SignData.lua

	for i,sec in ipairs( Credits ) do
		credSec = signBarn:AddCreditSection( sec.SectionName, #sec.Entries )
		for j,ent in ipairs( sec.Entries ) do
			textEnt = credSec:GetEntry( j-1 )
			signBarn:SetTextEntry( textEnt, ent.Text )  
		end
	end

-- SignInstances.lua
-- iterate through the sign instances and tie the signs to the credit entries

    --local color = { 255, 128, 64, 255 }

	for i,signInst in ipairs( SignInstances ) do
		if signInst.secName ~= "" then
			textEnt = signBarn:FindTextEntry( signInst.secName, signInst.entNum )
		else
			textEnt = signBarn:GetTextEntry( signInst.secNum, signInst.entNum )		
		end 
		
		local sign = signBarn:AddSignTextEntry( textEnt )
		SignInitialize( sign, signInst )
		
		Names[ signInst.ObjectName ] = sign
	end
	
	SignInstances = {}
	Credits = {}
end
