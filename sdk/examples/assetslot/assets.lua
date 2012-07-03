-- Metadata

IconAssets = group{quality=9.95}
Icon = image{"icon.png"}

-- Loading screen: Divided into two asset groups

BootstrapGroup = group{ quality=9.8 }
LoadingBg = image{ "images/loading.png" }

-- Main Menu

MenuGroup = group{ quality=9.8 }

MenuStripe = image{ "images/stripes.png", pinned=1 }
MenuBg = image{ "images/bg.png", pinned=1 }
MenuFooter = image{ "images/footer.png" }

MenuLabelEmpty = image{ "images/labelEmpty.png" }
MenuIconSpinny = image{ "images/icon-spinny.png" }
MenuIconBall = image{ "images/icon-ball.png" }

-- Animation: "spinny".

function frames(fmt, count, pingPong)
    t = {}
    for i = 1, count do
        t[1+#t] = string.format(fmt, i)
    end
    if pingPong then
        for i = count-1, 2, -1 do
            t[1+#t] = string.format(fmt, i)
        end
    end
    return t
end

SpinnyGroup = group{ quality=9.5 }
Spinny = image{ frames("images/spinny/spinny-%d.png", 24, true) }

-- Animation: "ball"

BallGroup = group{ quality=10 }
Ball = image{ frames("images/ball/%04d.png", 29) }
