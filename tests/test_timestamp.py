import libernet.plat.timestamp

def test_timestamp():
    assert libernet.plat.timestamp.create(libernet.plat.timestamp.TIMESTAMP_EPOCH) == 0.0
    assert libernet.plat.timestamp.convert(0) == libernet.plat.timestamp.TIMESTAMP_EPOCH
