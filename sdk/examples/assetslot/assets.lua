-- Loading screen: Divided into two asset groups

BootstrapGroup = group{ quality=10.0 }
LoadingBg = image{ "images/loading-bg.png" }

LoadingGroup = group{ quality=9.5 }
LoadingText = image{ "images/loading.png" }

-- A big animation, "spinny".

function frames(fmt, count)
    -- Loop the animation forward and backward.
    -- (This is cheap to do with compressed AssetImages)
    t = {}
    for i = 1, count do
        t[1+#t] = string.format(fmt, i)
    end
    for i = count-1, 2, -1 do
        t[1+#t] = string.format(fmt, i)
    end
    return t
end

SpinnyGroup = group{ quality=9.0 }
Spinny = image{ frames("images/spinny/spinny-%d.png", 24) }
