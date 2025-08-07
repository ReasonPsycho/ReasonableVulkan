//
// Created by redkc on 09.05.2025.
//

#ifndef UUIDMANAGER_HPP
#define UUIDMANAGER_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>

namespace am {
    class UUIDManager {
    public:
        static boost::uuids::uuid generate() {
            return generator();
        }

        static std::string toString(const boost::uuids::uuid &id) {
            return boost::uuids::to_string(id);
        }

        static boost::uuids::uuid fromString(const std::string &str) {
            boost::uuids::string_generator gen;
            return gen(str);
        }

    private:
        static boost::uuids::random_generator generator;
    };
}


#endif //UUIDMANAGER_HPP
