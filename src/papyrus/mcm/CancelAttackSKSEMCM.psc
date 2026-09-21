Scriptname CancelAttackSKSEMCM extends MCM_ConfigBase Hidden

Bool Property bCustomBindingSelected Auto Hidden
Bool Property bAdditionalCustomBindingSelected Auto Hidden

Bool Function SyncBindingDisplay()
    Bool changed = False
    Bool primaryCustom = GetModSettingInt("iBinding:General") == 2
    Bool additionalCustom = GetModSettingInt("iAdditionalBinding:General") == 3

    If primaryCustom != bCustomBindingSelected
        bCustomBindingSelected = primaryCustom
        changed = True
    EndIf
    If additionalCustom != bAdditionalCustomBindingSelected
        bAdditionalCustomBindingSelected = additionalCustom
        changed = True
    EndIf

    Return changed
EndFunction

Event OnConfigInit()
    SyncBindingDisplay()
EndEvent

Event OnConfigOpen()
    SyncBindingDisplay()
EndEvent

Event OnSettingChange(String a_ID)
    If (a_ID == "iBinding:General" || a_ID == "iAdditionalBinding:General") && SyncBindingDisplay()
        ForcePageReset()
    EndIf
EndEvent

Event OnConfigClose()
    CancelAttackSKSENative.ReloadSettings()
EndEvent
