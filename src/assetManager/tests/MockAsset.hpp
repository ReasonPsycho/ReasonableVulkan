// MockAsset.hpp
#ifndef MOCK_ASSET_HPP
#define MOCK_ASSET_HPP

#include "Asset.hpp"


namespace ae {
    class MockAsset : public Asset {
    public:
        explicit MockAsset(BaseFactoryContext baseFactoryContext) 
            : Asset(baseFactoryContext), testContent("test") {}
        
        [[nodiscard]] size_t calculateContentHash() const override {
            return std::hash<std::string>{}(testContent);
        }
        
        [[nodiscard]] AssetType getType() const override {
            return AssetType::Model;
        }
        
        void setContent(const std::string& content) {
            testContent = content;
        }
        
    private:
        std::string testContent;
    };
}

#endif