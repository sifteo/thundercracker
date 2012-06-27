Ball1Group = group{ quality=10 }
Ball1 = image{ "images/ball1.png", height=128 }

Ball2Group = group{ quality=8.0 }
Ball2 = image{ "images/ball2.png", width=128 }

Ball3Group = group{ quality=9.5 }
Ball3 = image{ "images/ball3.png", width=128 }

NumberList = {}
for n = 1,25 do
    _G['NumberGroup' .. n] = group{ quality=10 }
    NumberList[n] = image{ string.format("images/number-%d.png", n) }
end
