exe=$1
$exe < test0.in > test0.m.out
diff test0.out test0.m.out
status=$?
rm dict.txt test0.m.out -f
exit $status
