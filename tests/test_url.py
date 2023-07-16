#!/usr/bin/env python3


import libernet.url
from libernet.url import AES256, SHA256, PASSWORD, LIKE


PARSE_TEST_SET = (
    (f'/{SHA256}/1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86', '1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86', None, '1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86', SHA256),
    (f'/{SHA256}/1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86/{AES256}/ecabb29bbec22a6501c06aec21d8b36adfbaecd7fd57389a77563938d57ae2be', '1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86', 'ecabb29bbec22a6501c06aec21d8b36adfbaecd7fd57389a77563938d57ae2be', 'ecabb29bbec22a6501c06aec21d8b36adfbaecd7fd57389a77563938d57ae2be', AES256),
    (f'/{SHA256}/1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86/{PASSWORD}/ecabb29bbec22a6501c06aec21d8b36adfbaecd7fd57389a77563938d57ae2be', '1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86', 'ecabb29bbec22a6501c06aec21d8b36adfbaecd7fd57389a77563938d57ae2be', None, PASSWORD),
    (f'/{SHA256}/{LIKE}/1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86', '1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86', None, '1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86', LIKE),
)


def test_parse():
    for url, identifier, key, contents, kind in PARSE_TEST_SET:
        i, ke, c, ki = libernet.url.parse(url)
        assert identifier == i, f"{i} vs {identifier} for {url}"
        assert key == ke, f"{ke} vs {key} for {url}"
        assert contents == c, f"{c} vs {contents} for {url}"
        assert kind == ki, f"{ki} vs {kind} for {url}"


def test_url_construction():
    identifier = '1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86'
    key = 'ecabb29bbec22a6501c06aec21d8b36adfbaecd7fd57389a77563938d57ae2be'
    assert libernet.url.for_data_block(identifier) == f'/{SHA256}/{identifier}'
    assert libernet.url.for_data_block(identifier, like=True) == f'/{SHA256}/{LIKE}/{identifier}'
    assert libernet.url.for_data_block(identifier, like=False) == f'/{SHA256}/{identifier}'
    assert libernet.url.for_encrypted(identifier, key) == f'/{SHA256}/{identifier}/{AES256}/{key}'
    assert libernet.url.for_encrypted(identifier, key, PASSWORD) == f'/{SHA256}/{identifier}/{PASSWORD}/{key}'
    assert libernet.url.for_encrypted(identifier, key, AES256) == f'/{SHA256}/{identifier}/{AES256}/{key}'


if __name__ == "__main__":
    test_parse()
    test_url_construction()
