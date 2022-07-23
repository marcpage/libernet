import tempfile
import os

import libernet.plat.files

def test_symlink():
    test_contents = "testing data"
    test_file = 'data.txt'
    sym_file = 'symlink.txt'
    system = libernet.plat.files.SYSTEM
    test_systems = {
        "Darwin": True, 
        "Linux": True, 
        "Windows": False
    }

    for test_system in test_systems:
        libernet.plat.files.SYSTEM = test_system

        with tempfile.TemporaryDirectory() as working:
            contents_path = os.path.join(working, test_file)
            link_path = os.path.join(working, sym_file)
            
            with open(contents_path, 'w') as text_file:
                text_file.write(test_contents)

            assert libernet.plat.files.symlink(test_file, link_path) == test_systems[test_system]

            if test_systems[test_system]:
                assert os.path.islink(link_path)
                assert os.path.isfile(link_path)

                with open(link_path, 'r') as text_file:
                    assert test_contents == text_file.read()

    libernet.plat.files.SYSTEM = system