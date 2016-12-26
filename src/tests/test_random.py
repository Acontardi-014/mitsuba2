from mitsuba.core import PacketSize
import numpy as np


def test_pcg32():
    from mitsuba.core import pcg32

    values_32bit = [0xa15c02b7, 0x7b47f409, 0xba1d3330, 0x83d2f293,
                    0xbfa4784b, 0xcbed606e]
    values_coins = 'HHTTTHTHHHTHTTTHHHHHTTTHHHTHTHTHTTHTTTHHHHHHTT' \
                   'TTHHTTTTTHTTTTTTTHT'
    values_rolls = [3, 4, 1, 1, 2, 2, 3, 2, 4, 3, 2, 4, 3, 3, 5, 2, 3, 1, 3,
                    1, 5, 1, 4, 1, 5, 6, 4, 6, 6, 2, 6, 3, 3]
    values_cards = 'Qd Ks 6d 3s 3d 4c 3h Td Kc 5c Jh Kd Jd As 4s 4h Ad Th ' \
                   'Ac Jc 7s Qs 2s 7h Kh 2d 6c Ah 4d Qh 9h 6s 5s 2c 9c Ts ' \
                   '8d 9s 3c 8c Js 5d 2h 6h 7d 8s 9d 5h 8h Qc 7c Tc'

    p = pcg32()
    p.seed(42, 54)
    base = pcg32(p)

    for v in values_32bit:
        assert p.nextUInt() == v

    p.advance(-4)
    assert p - base == 2

    for v in values_32bit[2:]:
        assert p.nextUInt() == v

    for v in values_coins:
        assert 'H' if p.nextUInt(2) == 1 else 'T' == v

    for v in values_rolls:
        assert p.nextUInt(6) + 1 == v

    cards = list(range(52))
    p.shuffle(cards)

    number = ['A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J',
              'Q', 'K']
    suit = ['h', 'c', 'd', 's']
    ctr = 0

    for v in values_cards.split():
        assert v == number[cards[ctr] // len(suit)] + \
            suit[cards[ctr] % len(suit)]
        ctr += 1


def test_tea_single():
    from mitsuba.core import sampleTEASingle

    assert sampleTEASingle(1, 1, 4) == 0.5424730777740479
    assert sampleTEASingle(1, 2, 4) == 0.5079904794692993
    assert sampleTEASingle(1, 3, 4) == 0.4171961545944214
    assert sampleTEASingle(1, 4, 4) == 0.008385419845581055
    assert sampleTEASingle(1, 5, 4) == 0.8085528612136841
    assert sampleTEASingle(2, 1, 4) == 0.6939879655838013
    assert sampleTEASingle(3, 1, 4) == 0.6978365182876587
    assert sampleTEASingle(4, 1, 4) == 0.4897364377975464


def test_tea_double():
    from mitsuba.core import sampleTEADouble

    assert sampleTEADouble(1, 1, 4) == 0.5424730799533735
    assert sampleTEADouble(1, 2, 4) == 0.5079905082233922
    assert sampleTEADouble(1, 3, 4) == 0.4171962610608142
    assert sampleTEADouble(1, 4, 4) == 0.008385529523330604
    assert sampleTEADouble(1, 5, 4) == 0.80855288317879
    assert sampleTEADouble(2, 1, 4) == 0.6939880404156831
    assert sampleTEADouble(3, 1, 4) == 0.6978365636630994
    assert sampleTEADouble(4, 1, 4) == 0.48973647949223253


def test_tea_vectorized():
    from mitsuba.core import sampleTEASingle, sampleTEADouble

    result = sampleTEASingle(
        np.full(PacketSize, 1), np.arange(
            PacketSize, dtype=np.uint32), 4)
    for i in range(PacketSize):
        assert result[i] == sampleTEASingle(1, i, 4)

    result = sampleTEADouble(
        np.full(PacketSize, 1), np.arange(
            PacketSize, dtype=np.uint32), 4)
    for i in range(PacketSize):
        assert result[i] == sampleTEADouble(1, i, 4)
