/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

#include "MIMC52.hpp"

#include "AES.hpp"
#include "Constants.hpp"

namespace {

// Largest 1024 primes of form 6k + 5 and less than 2^52. Only the least significant 32
// bits need to be here, as the most significant bits are all 1.
const uint32_t ZT_MIMC52_PRIMES[1024] = {
    4294895267, 4294895477, 4294895513, 4294895519, 4294895543, 4294895567, 4294895657, 4294895711, 4294895777, 4294895861, 4294895909, 4294895921, 4294895969, 4294896011, 4294896149, 4294896227, 4294896401, 4294896473, 4294896527,
    4294896563, 4294896653, 4294896731, 4294896863, 4294896899, 4294896983, 4294897037, 4294897103, 4294897331, 4294897349, 4294897451, 4294897571, 4294897661, 4294897703, 4294897757, 4294897793, 4294897811, 4294897817, 4294897829,
    4294897877, 4294897919, 4294897991, 4294898027, 4294898129, 4294898153, 4294898231, 4294898273, 4294898279, 4294898291, 4294898363, 4294898369, 4294898417, 4294898423, 4294898453, 4294898489, 4294898573, 4294898579, 4294898639,
    4294898693, 4294898747, 4294898759, 4294898867, 4294898879, 4294898909, 4294898921, 4294898933, 4294899011, 4294899041, 4294899047, 4294899203, 4294899221, 4294899227, 4294899287, 4294899341, 4294899431, 4294899509, 4294899533,
    4294899539, 4294899551, 4294899629, 4294899791, 4294899809, 4294899971, 4294900001, 4294900007, 4294900013, 4294900307, 4294900331, 4294900427, 4294900469, 4294900481, 4294900541, 4294900583, 4294900781, 4294900853, 4294900931,
    4294900991, 4294901033, 4294901087, 4294901159, 4294901267, 4294901393, 4294901411, 4294901489, 4294901657, 4294902011, 4294902071, 4294902101, 4294902107, 4294902353, 4294902377, 4294902599, 4294902647, 4294902743, 4294902869,
    4294902977, 4294903067, 4294903103, 4294903259, 4294903289, 4294903397, 4294903421, 4294903493, 4294903577, 4294903631, 4294903637, 4294903733, 4294903799, 4294903823, 4294904003, 4294904033, 4294904081, 4294904129, 4294904279,
    4294904297, 4294904303, 4294904333, 4294904351, 4294904381, 4294904453, 4294904519, 4294904561, 4294904639, 4294904657, 4294904747, 4294904807, 4294904843, 4294905089, 4294905149, 4294905293, 4294905299, 4294905311, 4294905443,
    4294905479, 4294905539, 4294905623, 4294905641, 4294905671, 4294905707, 4294905887, 4294905977, 4294906091, 4294906103, 4294906139, 4294906157, 4294906223, 4294906259, 4294906487, 4294906493, 4294906523, 4294906547, 4294906553,
    4294906571, 4294906577, 4294906589, 4294906703, 4294906733, 4294906763, 4294906841, 4294906859, 4294906937, 4294907057, 4294907063, 4294907141, 4294907231, 4294907249, 4294907261, 4294907267, 4294907387, 4294907417, 4294907567,
    4294907603, 4294907699, 4294907789, 4294907849, 4294907873, 4294907879, 4294908023, 4294908071, 4294908119, 4294908209, 4294908227, 4294908329, 4294908491, 4294908503, 4294908569, 4294908653, 4294908713, 4294908719, 4294908791,
    4294908839, 4294908869, 4294908989, 4294909031, 4294909067, 4294909109, 4294909253, 4294909529, 4294909589, 4294909643, 4294909739, 4294909799, 4294909811, 4294909853, 4294910003, 4294910039, 4294910189, 4294910201, 4294910219,
    4294910273, 4294910333, 4294910369, 4294910393, 4294910471, 4294910549, 4294910651, 4294910669, 4294910681, 4294910711, 4294910753, 4294910801, 4294910981, 4294911053, 4294911143, 4294911227, 4294911239, 4294911359, 4294911383,
    4294911407, 4294911521, 4294911551, 4294911611, 4294911641, 4294911689, 4294911719, 4294911869, 4294912109, 4294912133, 4294912151, 4294912187, 4294912223, 4294912331, 4294912439, 4294912607, 4294912703, 4294912859, 4294912871,
    4294912907, 4294912961, 4294913003, 4294913111, 4294913309, 4294913333, 4294913357, 4294913399, 4294913411, 4294913459, 4294913501, 4294913531, 4294913591, 4294913609, 4294913663, 4294913783, 4294913819, 4294913903, 4294914137,
    4294914413, 4294914473, 4294914497, 4294914527, 4294914551, 4294914593, 4294914611, 4294914659, 4294914671, 4294914743, 4294914863, 4294914917, 4294915061, 4294915103, 4294915139, 4294915217, 4294915223, 4294915253, 4294915283,
    4294915373, 4294915433, 4294915607, 4294916069, 4294916213, 4294916267, 4294916303, 4294916393, 4294916441, 4294916477, 4294916507, 4294916573, 4294916633, 4294916687, 4294916783, 4294916837, 4294916897, 4294916921, 4294917029,
    4294917047, 4294917101, 4294917203, 4294917287, 4294917299, 4294917389, 4294917437, 4294917527, 4294917557, 4294917611, 4294917617, 4294917689, 4294917821, 4294917857, 4294917917, 4294917941, 4294918169, 4294918187, 4294918307,
    4294918409, 4294918433, 4294918481, 4294918703, 4294918709, 4294918733, 4294918799, 4294918871, 4294919009, 4294919249, 4294919279, 4294919291, 4294919363, 4294919381, 4294919441, 4294919447, 4294919549, 4294919579, 4294919633,
    4294919657, 4294919669, 4294919693, 4294919711, 4294920029, 4294920059, 4294920089, 4294920197, 4294920239, 4294920257, 4294920263, 4294920269, 4294920341, 4294920353, 4294920407, 4294920503, 4294920599, 4294920647, 4294920743,
    4294920803, 4294920809, 4294920881, 4294920899, 4294920983, 4294921043, 4294921139, 4294921151, 4294921181, 4294921229, 4294921289, 4294921331, 4294921343, 4294921391, 4294921469, 4294921709, 4294921721, 4294921823, 4294921847,
    4294921889, 4294922057, 4294922171, 4294922201, 4294922237, 4294922309, 4294922399, 4294922447, 4294922507, 4294922513, 4294922549, 4294922609, 4294922663, 4294922861, 4294922933, 4294923101, 4294923191, 4294923209, 4294923221,
    4294923251, 4294923263, 4294923359, 4294923371, 4294923377, 4294923461, 4294923521, 4294923953, 4294924001, 4294924091, 4294924121, 4294924319, 4294924397, 4294924571, 4294924583, 4294924751, 4294924817, 4294924823, 4294924847,
    4294924877, 4294925003, 4294925027, 4294925117, 4294925237, 4294925243, 4294925297, 4294925369, 4294925627, 4294925639, 4294925729, 4294925747, 4294925873, 4294925891, 4294925933, 4294926047, 4294926059, 4294926209, 4294926221,
    4294926233, 4294926257, 4294926329, 4294926371, 4294926401, 4294926413, 4294926437, 4294926563, 4294926569, 4294926917, 4294926923, 4294926947, 4294926971, 4294927067, 4294927073, 4294927151, 4294927349, 4294927367, 4294927403,
    4294927481, 4294927523, 4294927553, 4294927589, 4294927649, 4294927673, 4294927727, 4294927739, 4294927763, 4294927889, 4294928183, 4294928207, 4294928249, 4294928327, 4294928351, 4294928399, 4294928483, 4294928489, 4294928543,
    4294928597, 4294928951, 4294928963, 4294928981, 4294929017, 4294929059, 4294929161, 4294929197, 4294929233, 4294929269, 4294929311, 4294929323, 4294929341, 4294929383, 4294929401, 4294929497, 4294929509, 4294929581, 4294929707,
    4294929743, 4294930043, 4294930121, 4294930193, 4294930223, 4294930349, 4294930403, 4294930571, 4294930613, 4294930721, 4294930751, 4294930877, 4294930931, 4294930961, 4294930967, 4294930973, 4294931021, 4294931051, 4294931057,
    4294931063, 4294931219, 4294931273, 4294931339, 4294931423, 4294931441, 4294931453, 4294931567, 4294931639, 4294931717, 4294931897, 4294931969, 4294932023, 4294932053, 4294932239, 4294932299, 4294932443, 4294932671, 4294932677,
    4294932731, 4294932743, 4294932767, 4294932773, 4294932779, 4294932881, 4294932899, 4294932929, 4294933067, 4294933277, 4294933307, 4294933343, 4294933451, 4294933523, 4294933763, 4294933793, 4294933829, 4294933847, 4294933871,
    4294933997, 4294934033, 4294934111, 4294934207, 4294934243, 4294934267, 4294934279, 4294934291, 4294934327, 4294934363, 4294934423, 4294934489, 4294934561, 4294934867, 4294934921, 4294934969, 4294935137, 4294935239, 4294935299,
    4294935431, 4294935539, 4294935629, 4294935701, 4294935791, 4294935797, 4294935803, 4294935959, 4294936001, 4294936007, 4294936037, 4294936079, 4294936127, 4294936163, 4294936247, 4294936307, 4294936331, 4294936409, 4294936451,
    4294936601, 4294936607, 4294936619, 4294936667, 4294936709, 4294936733, 4294936751, 4294936763, 4294936829, 4294936937, 4294936997, 4294937027, 4294937051, 4294937093, 4294937177, 4294937213, 4294937291, 4294937381, 4294937417,
    4294937429, 4294937681, 4294937693, 4294937753, 4294937771, 4294937813, 4294937837, 4294937891, 4294937969, 4294938071, 4294938101, 4294938323, 4294938371, 4294938401, 4294938467, 4294938473, 4294938521, 4294938599, 4294938731,
    4294938779, 4294938833, 4294938899, 4294938977, 4294938983, 4294939067, 4294939127, 4294939223, 4294939277, 4294939331, 4294939337, 4294939391, 4294939457, 4294939559, 4294939673, 4294939691, 4294939901, 4294939991, 4294940087,
    4294940093, 4294940189, 4294940213, 4294940417, 4294940657, 4294940699, 4294940753, 4294940801, 4294940873, 4294940951, 4294941047, 4294941143, 4294941161, 4294941227, 4294941281, 4294941377, 4294941509, 4294941551, 4294941701,
    4294941731, 4294941767, 4294941911, 4294941923, 4294942043, 4294942139, 4294942313, 4294942343, 4294942373, 4294942427, 4294942529, 4294942601, 4294942649, 4294942673, 4294942679, 4294942733, 4294942769, 4294942811, 4294942961,
    4294943129, 4294943141, 4294943219, 4294943369, 4294943423, 4294943471, 4294943651, 4294943687, 4294943717, 4294943729, 4294943747, 4294943759, 4294943813, 4294943819, 4294943891, 4294944077, 4294944191, 4294944233, 4294944239,
    4294944353, 4294944389, 4294944581, 4294944623, 4294944629, 4294944659, 4294944821, 4294945031, 4294945157, 4294945211, 4294945229, 4294945301, 4294945337, 4294945343, 4294945511, 4294945547, 4294945667, 4294945709, 4294945757,
    4294945841, 4294945991, 4294946033, 4294946099, 4294946153, 4294946477, 4294946687, 4294946747, 4294946957, 4294946993, 4294947023, 4294947131, 4294947167, 4294947287, 4294947311, 4294947413, 4294947581, 4294947599, 4294947671,
    4294947851, 4294947959, 4294948067, 4294948073, 4294948193, 4294948259, 4294948421, 4294948451, 4294948613, 4294948673, 4294948883, 4294949027, 4294949057, 4294949069, 4294949519, 4294949531, 4294949603, 4294949609, 4294949627,
    4294949693, 4294949729, 4294949741, 4294949807, 4294949921, 4294949939, 4294949981, 4294949993, 4294950083, 4294950173, 4294950197, 4294950251, 4294950287, 4294950317, 4294950323, 4294950329, 4294950581, 4294950593, 4294950617,
    4294950629, 4294950713, 4294950929, 4294951151, 4294951163, 4294951169, 4294951379, 4294951583, 4294951613, 4294951853, 4294951907, 4294951913, 4294951937, 4294951961, 4294952063, 4294952183, 4294952393, 4294952543, 4294952549,
    4294952597, 4294952627, 4294952687, 4294952723, 4294952729, 4294952789, 4294952819, 4294952873, 4294952891, 4294952903, 4294952969, 4294952999, 4294953023, 4294953107, 4294953173, 4294953281, 4294953341, 4294953431, 4294953599,
    4294953689, 4294953719, 4294953827, 4294953887, 4294953977, 4294954073, 4294954079, 4294954157, 4294954217, 4294954283, 4294954607, 4294954667, 4294954859, 4294954901, 4294954973, 4294955081, 4294955237, 4294955273, 4294955327,
    4294955441, 4294955507, 4294955591, 4294955789, 4294955831, 4294955837, 4294955927, 4294955963, 4294955969, 4294955987, 4294956041, 4294956047, 4294956197, 4294956323, 4294956359, 4294956551, 4294956593, 4294956623, 4294956629,
    4294956641, 4294956719, 4294956761, 4294956767, 4294956797, 4294956821, 4294956833, 4294957037, 4294957079, 4294957103, 4294957181, 4294957349, 4294957379, 4294957433, 4294957463, 4294957511, 4294957577, 4294957727, 4294957859,
    4294957877, 4294958039, 4294958153, 4294958309, 4294958417, 4294958441, 4294958693, 4294958717, 4294958753, 4294958903, 4294958909, 4294959017, 4294959071, 4294959107, 4294959161, 4294959257, 4294959299, 4294959329, 4294959431,
    4294959593, 4294959599, 4294959659, 4294959893, 4294959917, 4294959983, 4294960001, 4294960031, 4294960061, 4294960079, 4294960097, 4294960271, 4294960283, 4294960349, 4294960367, 4294960421, 4294960529, 4294960541, 4294960583,
    4294960613, 4294960673, 4294960691, 4294960697, 4294960787, 4294960919, 4294961003, 4294961039, 4294961153, 4294961159, 4294961171, 4294961321, 4294961411, 4294961471, 4294961507, 4294961537, 4294961669, 4294961717, 4294961741,
    4294961873, 4294962059, 4294962137, 4294962167, 4294962263, 4294962281, 4294962311, 4294962341, 4294962413, 4294962521, 4294962563, 4294962761, 4294962893, 4294963103, 4294963163, 4294963223, 4294963313, 4294963349, 4294963427,
    4294963547, 4294963559, 4294963721, 4294963799, 4294963817, 4294963901, 4294963919, 4294964021, 4294964279, 4294964297, 4294964363, 4294964387, 4294964411, 4294964567, 4294964603, 4294964687, 4294964777, 4294965041, 4294965071,
    4294965119, 4294965221, 4294965251, 4294965287, 4294965413, 4294965569, 4294965647, 4294965671, 4294965689, 4294965779, 4294965839, 4294965893, 4294966091, 4294966109, 4294966127, 4294966157, 4294966187, 4294966199, 4294966211,
    4294966403, 4294966457, 4294966499, 4294966541, 4294966637, 4294966661, 4294966739, 4294966823, 4294966883, 4294966901, 4294966961, 4294967027, 4294967087, 4294967099, 4294967123, 4294967153, 4294967249
};

#ifdef ZT_NO_IEEE_DOUBLE

/* Integer 64-bit modular multiply for systems without an IEEE FPU. This is
 * much slower on systems that can use the FPU hack. */
static uint64_t mulmod64(uint64_t a, uint64_t b, const uint64_t m)
{
    uint64_t res = 0;
    while ((a)) {
        if ((a << 63U))
            res = (res + b) % m;
        a >>= 1U;
        b = (b << 1U) % m;
    }
    return res;
}

#define mulmod52(a, b, m, mf) mulmod64((a), (b), (m))

#else

/* If we have IEEE double precision FPU support, we can do this trick to
 * execute a 52-bit mulmod faster than the integer ALU. This trick is why
 * this is a 52-bit work function and not a 64-bit work function, as it
 * performs fairly equally across CPUs. */
ZT_INLINE uint64_t mulmod52(uint64_t a, const uint64_t b, const uint64_t m, const double mf)
{
    a = ((a * b) - (((uint64_t)(((double)a * (double)b) / mf) - 1) * m));
    // a -= m * (uint64_t)(a > m); // faster on some systems, but slower on newer cores
    a %= m;
    return a;
}

#endif

// Compute a^e%m (mf is m in floating point form to avoid repeated conversion)
ZT_INLINE uint64_t modpow52(uint64_t a, uint64_t e, const uint64_t m, const double mf)
{
    uint64_t res = 1ULL;
    for (;;) {
        if ((e << 63U)) {
            res = mulmod52(res, a, m, mf);
        }
        if (likely((e >>= 1U) != 0)) {
            a = mulmod52(a, a, m, mf);
        }
        else {
            break;
        }
    }
    return res;
}

static const ZeroTier::AES s_mimc52AES("abcdefghijklmnopqrstuvwxyz012345");

// This fills k[] with pseudorandom bytes from the challenge.
// This doesn't have to be non-reversible or secure, just strongly random.
ZT_INLINE void fillK(uint64_t k[34], const uint8_t challenge[32])
{
    s_mimc52AES.encrypt(challenge, k);
    s_mimc52AES.encrypt(challenge + 16, k + 2);
    k[2] ^= k[0];
    k[3] ^= k[1];
    for (unsigned int i = 2, j = 4; j < 34; i += 2, j += 2)
        s_mimc52AES.encrypt(k + i, k + j);

#if __BYTE_ORDER == __BIG_ENDIAN
    for (unsigned int i = 0; i < 34; ++i)
        k[i] = Utils::swapBytes(k[i]);
#endif
}

}   // anonymous namespace

namespace ZeroTier {
namespace MIMC52 {

uint64_t delay(const uint8_t challenge[32], const unsigned long rounds)
{
    uint64_t k[34];
    fillK(k, challenge);

    const uint64_t p = 0x000fffff00000000ULL | (uint64_t)ZT_MIMC52_PRIMES[((unsigned long)k[32]) & 1023];
    const uint64_t e = ((p * 2ULL) - 1ULL) / 3ULL;
    const uint64_t m52 = 0xfffffffffffffULL;
    const double pf = (double)p;

    uint64_t x = k[33] % p;
    for (unsigned long r = 0, kn = rounds; r < rounds; ++r) {
        x = (x - k[--kn & 31]) & m52;
        x = modpow52(x, e, p, pf);
    }

    return x;
}

bool verify(const uint8_t challenge[32], const unsigned long rounds, uint64_t proof)
{
    uint64_t k[34];
    fillK(k, challenge);

    const uint64_t p = 0x000fffff00000000ULL | (uint64_t)ZT_MIMC52_PRIMES[((unsigned long)k[32]) & 1023];
    const uint64_t m52 = 0xfffffffffffffULL;
    const double pf = (double)p;

    for (unsigned long r = 0; r < rounds; ++r) {
        const uint64_t kk = k[r & 31];
        proof = mulmod52(mulmod52(proof, proof, p, pf), proof, p, pf);   // y = y ^ 3
        proof = (proof + kk) & m52;
    }

    return ((proof % p) == (k[33] % p));
}

}   // namespace MIMC52
}   // namespace ZeroTier
