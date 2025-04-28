/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "cliParams.hh"
#include "common/constants.hh"
#include "constants.hh"
#include "data/acqParams.hh"
#include "input/usb/usbParams.hh"
#include <argparse/argparse.hpp>
#include <chrono>

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace cli {

void parse (int argc, char **argv)
{
        auto &acquisitionParams = acq::priv::params ();
        auto &cliParams = cli::priv::params ();
        auto &usbParams = usb::priv::params ();

        argparse::ArgumentParser program ("logic-cli", "0.0.1");

        program.add_argument ("-s")
                .help ("Digital sample rate [Hz].")
                // .default_value (acq::DEFAULT_DIGITAL_SAMPLE_RATE)
                .store_into (acquisitionParams.digitalSampleRate);

        program.add_argument ("-c").help ("Number of digital channels.") /* .default_value (8) */.store_into (acquisitionParams.digitalChannels);

        program.add_argument ("-S")
                .help ("Analog sample rate [Hz].")
                // .default_value (acq::DEFAULT_ANALOG_SAMPLE_RATE)
                .store_into (acquisitionParams.analogSampleRate);
        program.add_argument ("-C").help ("Number of analog channels.") /* .default_value (0) */.store_into (acquisitionParams.analogChannels);

        program.add_argument ("-b", "--bytes")
                .help ("Limit the amount of bytes acquired [bytes].")
                .default_value (0UZ)
                .store_into (cliParams.bytes);

        program.add_argument ("-t", "--time").help ("Limit the acquisition time [seconds].").default_value (0UZ).store_into (cliParams.seconds);

        program.add_argument ("--usb_transfer")
                .help ("Set the bulk USB transfer size.")
                .default_value (DEFAULT_USB_TRANSFER_SIZE_B)
                .store_into (usbParams.usbTransfer);

        program.add_argument ("--usb_block")
                .help ("Set the USB and DMA block size.")
                .default_value (DEFAULT_USB_BLOCK_SIZE_B)
                .store_into (usbParams.usbBlock);

        auto &group = program.add_mutually_exclusive_group ();
        group.add_argument ("--fixed_buffer").help ("Do not acquire anything, send a fixed buffer of consecutive numbers.").flag ();
        group.add_argument ("--decimation").help ("Developer option.").flag ();

        program.parse_args (argc, argv);

        if (program.get<bool> ("--fixed_buffer")) {
                acquisitionParams.mode = common::acq::Mode::fixedBuffer;
        }
        else if (program.get<bool> ("--decimation")) {
                acquisitionParams.mode = common::acq::Mode::decimation4_1;
        }
}

} // namespace cli