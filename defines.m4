changequote(<!, !>)dnl
define(<!for!>,<!ifelse($#,0,<!<!$0!>!>,<!ifelse(eval($2<=$3),1,
    <!pushdef(<!$1!>,$2)$4<!!>popdef(<!$1!>)$0(<!$1!>,incr($2),$3,<!$4!>)!>)!>)!>)dnl
dnl
define(<!foreach!>,<!ifelse(eval($#>2),1,
    <!pushdef(<!$1!>,<!$3!>)$2<!!>popdef(<!$1!>)dnl
  <!!>ifelse(eval($#>3),1,<!$0(<!$1!>,<!$2!>,shift(shift(shift($@))))!>)!>)!>)dnl
dnl