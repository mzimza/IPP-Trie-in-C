# deklarujemy plik wykonywalny tworzony na podstawie odpowiedniego pliku źródłowego
add_executable (dict-editor dict-editor.c)

# przy kompilacji programu należy dołączyć bibliotekę
target_link_libraries (dict-editor dictionary vector)

# ustawiamy zmienną wskazującą na lokalizację folderu z testami do dict-editora
set (testdir ${CMAKE_SOURCE_DIR}/../tests/dict-editor)

#dodajemy test, który uruchomi skrypt w folderze z testami podając jako argument ścieżkę do wykonywalnego dict-editora
add_test(NAME dict-editor_global_test COMMAND
   ${testdir}/test.sh $<TARGET_FILE:dict-editor>
   WORKING_DIRECTORY ${testdir}
)
