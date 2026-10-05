-- new_script.lua

function OnCreate()
    print("[Script] new_script initialized")
end

function OnUpdate(dt)
    -- Update logic
end

function OnCollision(other)
    
end

-- Handler for Button_3
function On_Button_3_Clicked()
    print("Button_3 clicked!")
end

function OnButtonClicked(id)
    if id == "Button_3" then
        On_Button_3_Clicked()
    end
end
