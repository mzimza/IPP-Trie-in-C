# dodajemy bibliotekę vector, stworzoną na podstawie pliku vector.c
# biblioteka będzie dołączana statycznie (czyli przez linkowanie pliku .o)

add_library (vector vector.c)
#add_executable(vector vector.c)

if (CMOCKA)
    # dodajemy plik wykonywalny z testem    
    add_executable (vector_test vector_test.c)

	set_target_properties(vector_test PROPERTIES COMPILE_DEFINITIONS UNIT_TESTING=1)
	
    # i linkujemy go z biblioteką do testowania
    target_link_libraries (vector_test ${CMOCKA})

    # wreszcie deklarujemy, że to test
    add_test (vector_unit_test vector_test)
endif (CMOCKA)
