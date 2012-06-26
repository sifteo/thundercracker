BallGroup = group{ quality=10 }
Ball = image{ "images/ball.png", height=128 }

NumberList = {}

for n = 1,25 do
    _G['NumberGroup' .. n] = group{ quality=10 }
    NumberList[n] = image{ string.format("images/number-%d.png", n) }
end
