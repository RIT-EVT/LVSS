//
// Created by eaton on 2/22/2024.
//

#include <LVSS/dev/TPS2HB50BQ1.hpp>

namespace LVSS {

TPS2HB50BQ1::TPS2HB50BQ1(IO::GPIO& en, IO::GPIO& fault, IO::GPIO& status, IO::GPIO& in, IO::GPIO& out, IO::GPIO& diagEn, IO::GPIO& diagSelect1, IO::GPIO& diagSelect2)
    : EN(en), FAULT(fault), STATUS(status), IN(in), OUT(out), DIAG_EN(diagEn), DIAG_SELECT_1(diagSelect1), DIAG_SELECT_2(diagSelect2) {
}


}
