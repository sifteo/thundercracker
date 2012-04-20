#include "assets.gen.h"
#include "Skins.h"
#include "Game.h"
#include "AudioPlayer.h"

namespace TotalsGame
{
    namespace Skins
    {
        
#define SKIN(name) \
{\
Skin_##name##_VaultDoor,\
Skin_##name##_Background,\
Skin_##name##_Background_Lit,\
Skin_##name##_Accent,\
Skin_##name##_Lit_Bottom,\
Skin_##name##_Lit_Bottom_Add,\
Skin_##name##_Lit_Bottom_Sub,\
Skin_##name##_Lit_Bottom_Mul,\
Skin_##name##_Lit_Bottom_Div,\
Skin_##name##_Lit_Left,\
Skin_##name##_Lit_Right,\
Skin_##name##_Lit_Right_Add,\
Skin_##name##_Lit_Right_Sub,\
Skin_##name##_Lit_Right_Mul,\
Skin_##name##_Lit_Right_Div,\
Skin_##name##_Lit_Top,\
Skin_##name##_Unlit_Bottom,\
Skin_##name##_Unlit_Bottom_Add,\
Skin_##name##_Unlit_Bottom_Sub,\
Skin_##name##_Unlit_Bottom_Mul,\
Skin_##name##_Unlit_Bottom_Div,\
Skin_##name##_Unlit_Left,\
Skin_##name##_Unlit_Right,\
Skin_##name##_Unlit_Right_Add,\
Skin_##name##_Unlit_Right_Sub,\
Skin_##name##_Unlit_Right_Mul,\
Skin_##name##_Unlit_Right_Div,\
Skin_##name##_Unlit_Top\
}
        
        const Skin skins[NumSkins] =
        {
            SKIN(Default),
            SKIN(blue),
            SKIN(green),
            SKIN(purple),
            SKIN(red),
            SKIN(turquoise)
        };
        
        
        SkinType currentSkin = SkinType_Default;
        
        void SetSkin(SkinType skinType)
        {
            if(skinType == currentSkin)
                return;
            
            currentSkin = skinType;
            const Skin& s = GetSkin();
            
            //animate transition to new skin
            for(int c = 0; c < NUM_CUBES; c++)
            {
                AudioPlayer::PlayShutterOpen();
                TotalsCube *cube = Game::cubes+c;
                
                const float totalDuration = 0.3f;
                const float halfDuration = totalDuration / 2;
                
                SystemTime start = SystemTime::now();
                SystemTime now = start;
                
                //bring in from top and bottom
                do 
                {
                    TimeDelta d = now - start;
                    
                    //by our exit condition, d will never equal halfduration.  max actual frame is 9.
                    int frame = 10.0f * (float)d / halfDuration;
                    
                    cube->ClipImage(&s.vault_door, vec(0, -18 + frame));
                    cube->ClipImage(&s.vault_door, vec(0, 16 - frame));
                    System::paint();
                    
                    now = SystemTime::now();
                } while (now-start < halfDuration);
                
                //slide it over a bit        
                start += halfDuration;
                do 
                {
                    TimeDelta d = now - start;
                    
                    //by our exit condition, d will never equal halfduration.  max actual frame is 9.
                    int frame = 10.0f * (float)d / halfDuration;                   

                    cube->ClipImage(&s.vault_door, vec(-frame, -9));
                    cube->ClipImage(&s.vault_door, vec(-frame, 7));
                    
                    cube->ClipImage(&s.vault_door, vec(16-frame, -9));
                    cube->ClipImage(&s.vault_door, vec(16-frame, 7));
                    System::paint();
                    
                    now = SystemTime::now();
                } while (now-start < halfDuration);
                
                //make sure the door is fully in place
                cube->ClipImage(&s.vault_door, vec(-9, -9));
                cube->ClipImage(&s.vault_door, vec(-9, 7));
                
                cube->ClipImage(&s.vault_door, vec(16-9, -9));
                cube->ClipImage(&s.vault_door, vec(16-9, 7));
                System::paint();

                
            }
        }
        
        const Skin& GetSkin()
        {
            return skins[currentSkin];
        }
        
    }
}







