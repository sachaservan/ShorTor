#ifndef FILTER_HPP
#define FILTER_HPP

/** @file */
#include "relay.hpp"

/** 
 * @enum EffectType
 * Indicator of the type of filter effect. ADD, MUL, SET
 * stand for additive, multiplicative or set-value effect
 * respectively. A composed filter has effect type COMP.
 */
enum EffectType
{
    ADD,
    MUL,
    SET,
    COMP
};

/**
 * A Filter adjusts the cost of a relay to an adversary based on some
 * property of the relay. It is composed of a selection according to
 * some property of the relay and the resulting effect.
 * @see Relay
 */
class Filter
{
    public:

        /**
         * Constructor for Filter base type.
         * @param type The filter type. @see EffectType.
         */
        Filter(EffectType type)
            :
                type(type)
            {}

        /**
         * Composes two filters into a new one. 
         * The predicates are AND connected and the
         * effect of the righthand operand is evaluated
         * first, so in general, this is not commutative.
         * @param f Filter whose effect is applied first.
         * @return Filter  
         */ 
        Filter operator*(const Filter& f);

        /**
         * Applies filter methods successively to a given relay.
         * @param r The relay to which the filters are applied.
         * @param current Current cost of relay r.
         */
        double apply_filter(const Relay &r, double current);

        /**
         * Prints information about the filter.
         */
        void print();
            
    protected:

        /**
         * Predicate which determines if the filter should be applied
         * to a particualar relay.
         * @param r The relay which is tested.
         */
        virtual bool applicable(const Relay& r);

        /**
         * Calculation of new relay cost. 
         * @param current Current cost of the relay.
         */
        virtual double effect(double current);

        EffectType type; /**< The filter's effect type. @see effect_type */
};


/**
 * Simple Filters have a basic EffectType and an associated
 * cost cost_manipulator.
 */
class SimpleFilter : public Filter
{
    public:
        /**
        * Constructor for a simple filter
        * @param type Filter type. Should be one of MUL,ADD,SET.
        * @param cost_manipulator value associated to effect.
        */
        SimpleFilter(EffectType type, double cost_manipulator)
            :
                Filter(type),
                cost_manipulator(cost_manipulator)
            {}
    protected:
        double cost_manipulator; /**< Value associated to MUL, ADD or SET effect. */
};

/**
 * Country Filters apply a basic effect based on a relay's
 * country information.
 */
class CountryFilter : public SimpleFilter
{
    public:
        /** Creates a country filter which applies a basic effect based on a relay's
         * country information.
         * @param country Target country string.
         * @param type desired effect type.
         * @param cost_manipulator manipulator value of the effect.
         */
        CountryFilter(std::string country, EffectType type, double cost_manipulator) 
            : 
                SimpleFilter(type,cost_manipulator),
                country(country)
            {}

    private:
        std::string country; /**< The filter works on this country string. */
};

/**
 * Bandwidth Filters are multiplicative effects dependent on a relay's
 * specified bandwidth.
 */
class BandwidthFilter : public SimpleFilter
{
    public:
        /**
         * Creates a Bandwidth filter, which has a multiplicative effect weighted by
         * a relay's bandwidth.
         * @param cost_manipulator factor which is weighted by relay bandwidth.
         */
        BandwidthFilter(double cost_manipulator)
            :
                SimpleFilter(MUL, cost_manipulator)
            {}

};

/**
 * Simple Filter which has an effect based on a relay's
 * autonomous system (AS) information.
 */
class ASFilter : public SimpleFilter
{
    public:
        /** Constructor for an AS Filter.
         * @param as_identifier AS name or AS number.
         * @param type desired effect type.
         * @param cost_manipulator manipulator value of the effect.
         */
        ASFilter(std::string as_identifier, EffectType type, double cost_manipulator)
            : 
                SimpleFilter(type,cost_manipulator),
                as_identifier(as_identifier)
            {}
    private:
        std::string as_identifier;
};

/**
 * Simple Filter which has an effect based on a relay's
 * geographical location.
 */
class LocationFilter : public SimpleFilter{
    public:
        /** Constructor for a Geolocation Filter.
         * @param longitude target longitude.
         * @param latitude target latitude.
         * @param type desired effect type.
         * @param cost_manipulator manipulator value of the effect.
         */
        LocationFilter(float longitude, float latitude, EffectType type, double cost_manipulator)
            : 
                SimpleFilter(type,cost_manipulator),
                longitude(longitude),
                latitude(latitude)
            {}
    private:
            float longitude;
            float latitude;
};

/**
 * Simple Filter which has an effect based on a relay's
 * specified flags as compared to some test flags.
 */
class FlagFilter : public SimpleFilter{
    public:
        /** Constructor for a flag Filter.
         * @param flags test flags.
         * @param type desired effect type.
         * @param cost_manipulator manipulator value of the effect.
         */
        FlagFilter(int flags, EffectType type, double cost_manipulator)
            : 
                SimpleFilter(type,cost_manipulator),
                flags(flags)
            {}
    private:
        int flags;
};

/**
 * Simple Filter which has an effect based on a relay's
 * date of publication.
 */
class DateFilter : public SimpleFilter{
    public:
        /** Constructor for a publication date Filter.
         * @param date comparison date string.
         * @param type desired effect type.
         * @param cost_manipulator manipulator value of the effect.
         */
        DateFilter(std::string date, EffectType type, double cost_manipulator) //FIXME: use proper date format instead of string
            : 
                SimpleFilter(type,cost_manipulator),
                date(date)
            {}
    private:
            std::string date;
};

/**
 * Simple Filter which has an effect based on a relay's
 * policies as compared to some other policy.
 */
class PolicyFilter : public SimpleFilter{
    public:
        /** Constructor for a Policy Filter.
         * @param policy comparison policy.
         * @param type desired effect type.
         * @param cost_manipulator manipulator value of the effect.
         */
        PolicyFilter(const Relay::PolicyDescriptor policy, EffectType type, double cost_manipulator)
            : 
                SimpleFilter(type,cost_manipulator),
                policy(policy)
            {}
    private:
        Relay::PolicyDescriptor policy;
};

/**
 * Simple Filter which has an effect based on a relay's
 * Tor version information.
 */
class VersionFilter: public SimpleFilter{
    public:
        /** Constructor for a Tor Version Filter.
         * @param version version test string.
         * @param type desired effect type.
         * @param cost_manipulator manipulator value of the effect.
         */
        VersionFilter(std::string version, EffectType type, double cost_manipulator)
            : 
                SimpleFilter(type,cost_manipulator),
                version(version)
            {}
    private:
        std::string version;
};

/**
 * Simple Filter which has an effect based on a relay's
 * platform information.
 */
class PlatformFilter: public SimpleFilter{
    public:
        /** Constructor for a Platform Filter.
         * @param platform platform test string.
         * @param type desired effect type.
         * @param cost_manipulator manipulator value of the effect.
         */
        PlatformFilter(std::string platform, EffectType type, double cost_manipulator)
            : 
                SimpleFilter(type,cost_manipulator),
                platform(platform)
            {}
    private:
        std::string platform;
};

#endif
