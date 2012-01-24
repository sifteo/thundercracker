define i32 @answer() {
    %1 = add i32 2, 40
    ret i32 %1
}

;define void @siftmain()  {
;    br label %foo
;foo:
;    tail call void @_SYS_paint()
;    br label %foo
;}

declare void @_SYS_paint()
