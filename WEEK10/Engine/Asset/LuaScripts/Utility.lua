-- utility.lua

-- 코루틴 내에서 지정된 시간(초) 동안 대기
function Wait(seconds)
    if not coroutine.running() then
        error("Wait() can only be called inside a coroutine")
        return
    end

    coroutine.yield(seconds)
end

-- 코루틴 내에서 1프레임 대기
function WaitFrame()
    if not coroutine.running() then
        error("WaitFrame() can only be called inside a coroutine")
        return
    end
    
    coroutine.yield()
end

-- C++의 EKeyInput enum class와 1:1로 매칭되는 테이블
Keys = {
    -- 이동 키
    W = 0,
    A = 1,
    S = 2,
    D = 3,
    Q = 4,
    E = 5,
    R = 6,
    F = 7,
    P = 8,
    G = 9,

    -- 화살표 키
    Up = 10,
    Down = 11,
    Left = 12,
    Right = 13,

    -- 액션 키
    Space = 14,
    Enter = 15,
    Esc = 16,
    Tab = 17,
    Shift = 18,
    Ctrl = 19,
    Alt = 20,

    -- 숫자 키
    Num0 = 21,
    Num1 = 22,
    Num2 = 23,
    Num3 = 24,
    Num4 = 25,
    Num5 = 26,
    Num6 = 27,
    Num7 = 28,
    Num8 = 29,
    Num9 = 30,

    -- 마우스
    MouseLeft = 31,
    MouseRight = 32,
    MouseMiddle = 33,

    -- 기타
    Backtick = 34,
    F1 = 35,
    F2 = 36,
    F3 = 37,
    F4 = 38,
    Backspace = 39,
    Delete = 40,

    End = 41
}

CollisionTag = {
    None = 0,
    Player = 1,
    Enemy = 2,
    Wall = 3,
    Score = 4,
    Clear = 5
}