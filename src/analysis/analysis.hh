/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "common/params.hh"
#include "types.hh"

namespace logic::an {

struct ICheck {
        ICheck () = default;
        ICheck (ICheck const &) = default;
        ICheck &operator= (ICheck const &) = default;
        ICheck (ICheck &&) noexcept = default;
        ICheck &operator= (ICheck &&) noexcept = default;
        virtual ~ICheck () = default;

        /// Pre analysis
        virtual void start () = 0;
        /// For every block
        virtual void runRaw (data::RawData const &rd) = 0;
        virtual void run (data::SampleData const &bmd) = 0;
        /// Post analysis
        virtual void stop () = 0;
};

/**
 * A helper class
 */
class AbstractCheck : public ICheck {
public:
        AbstractCheck (size_t dmaBlockLenB) : dmaBlockLenB_{dmaBlockLenB} {}
        AbstractCheck (AbstractCheck const &) = default;
        AbstractCheck &operator= (AbstractCheck const &) = default;
        AbstractCheck (AbstractCheck &&) noexcept = default;
        AbstractCheck &operator= (AbstractCheck &&) noexcept = default;
        virtual ~AbstractCheck () = default;

        // Usually working with the raw data (before reorder algorithm) is not performed.
        void runRaw (data::RawData const &rd) override {}

        size_t dmaBlockLenB () const { return dmaBlockLenB_; }

private:
        size_t dmaBlockLenB_;
};

/**
 * @brief Perform the (debug) analysis using a strategy.
 *
 * @param rawData Data to analyze.
 * @param strategy Describes what to do.
 * @param discard Whether to discard the data or sotre it indefinitely in the RAM.
 */
void analyze (common::acq::Params const &params, data::Session *session, an::ICheck *strategy, bool discard = false, bool decompress = false);

} // namespace logic::an