define void @siftmain()  {
    br label %foo
foo:
    tail call void @_SYS_paint()
    br label %foo
}

declare void @_SYS_paint()
