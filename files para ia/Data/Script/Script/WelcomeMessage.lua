-- #############################################################################
-- # SSeMU MU Online Emulator
-- # ---------------------------------------------------------------------------
-- # Official Website : https://www.ssemu.com.ar
-- # YouTube          : https://www.youtube.com/@ssemu
-- # WhatsApp Channel : https://whatsapp.com/channel/0029VaATlSF8F2pDP6OlCG2z
-- # ---------------------------------------------------------------------------
-- # Copyright © SetecSoft Development. All rights reserved.
-- #############################################################################

-- ===========================================================================
BridgeFunctionAttach('OnCharacterEntry','WelcomeMessage_OnCharacterEntry')
-- ===========================================================================

function WelcomeMessage_OnCharacterEntry(aIndex)
	
	local UserName = GetObjectName(aIndex)
	
	local UserAccountLevel = GetObjectAccountLevel(aIndex)
	
	local UserAccountExpireDate = GetObjectAccountExpireDate(aIndex)
	
	local UserLang = GetObjectLang(aIndex)

	NoticeSend(aIndex,0,string.format(MessageGet(154,UserLang),UserName))

	if UserAccountLevel == 0 then 
		
		NoticeSend(aIndex,1,string.format(MessageGet(155,UserLang),UserAccountExpireDate))
		
	elseif UserAccountLevel == 1 then
		
		NoticeSend(aIndex,1,string.format(MessageGet(156,UserLang),UserAccountExpireDate))
		
	elseif UserAccountLevel == 2 then
		
		NoticeSend(aIndex,1,string.format(MessageGet(157,UserLang),UserAccountExpireDate))
		
	elseif UserAccountLevel == 3 then
		
		NoticeSend(aIndex,1,string.format(MessageGet(158,UserLang),UserAccountExpireDate))
		
	end
	
	MessageSend(aIndex,2,2,"Powered by SSeMU Emulator")
	
	MessageSend(aIndex,2,6,"https://www.ssemu.com.ar")
	
end