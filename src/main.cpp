#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include "gdstructs.hpp"
#include "compat_defs.hpp"

using namespace geode::prelude;

class Level {
    private:
    std::vector<BlockObject> m_blocks;
    std::vector<BackgroundChange> m_backgrounds;
    std::vector<GravityChange> m_gravity;
    std::vector<BlocksRise> m_rising;
    std::vector<BlocksFall> m_falling;
    int m_endPos = 3015;
    bool m_loaded = false;
    
    int readInt(ByteVector const& data, size_t& offset) {
        if (offset + 4 > data.size()) return -1;
        
        uint32_t b1 = static_cast<uint32_t>(data[offset]);
        uint32_t b2 = static_cast<uint32_t>(data[offset + 1]);
        uint32_t b3 = static_cast<uint32_t>(data[offset + 2]);
        uint32_t b4 = static_cast<uint32_t>(data[offset + 3]);
        
        offset += 4;
        return static_cast<int>((b1 << 24) | (b2 << 16) | (b3 << 8) | b4);
    }
    
    public:
    Level(std::filesystem::path path) {
        auto result = file::readBinary(path);
        if (result.isErr()) return;
        
        auto data = result.unwrap();
        if (data.size() < 10) return;
        
        size_t offset = 0;
        
        int formatVer = readInt(data, offset);
        
        uint8_t customGraphicsUnused = data[offset++];
        
        int numBlocks = (static_cast<int>(data[offset]) << 8) | static_cast<int>(data[offset + 1]);
        offset += 2;
        
        for (int i = 0; i < numBlocks; i++) {
            BlockObject obj;
            uint8_t type = data[offset++]; 
            obj.objType = static_cast<int>(type);
            obj.indexInVec = i;
            
            
            obj.xPos = readInt(data, offset);
            obj.yPos = readInt(data, offset); 
            
            m_blocks.push_back(obj);
        }
        
        m_endPos = readInt(data, offset);
        
        int numBG = readInt(data, offset);
        for (int i = 0; i < numBG; i++) {
            int x = readInt(data, offset);
            uint8_t isCustom = data[offset++];
            
            if (isCustom == 0) {
                int colorID = readInt(data, offset);
                m_backgrounds.push_back({x, colorID, nullptr, false, ""});
            } else {
                int strLen = (static_cast<int>(data[offset]) << 8) | static_cast<int>(data[offset + 1]);
                offset += 2;
                std::string texturePath;
                if (offset + strLen <= data.size()) {
                    texturePath = std::string(reinterpret_cast<const char*>(data.data() + offset), strLen);
                }
                offset += strLen;
                m_backgrounds.push_back({x, 0, nullptr, true, texturePath});
            }
        }
        
        int numGrav = readInt(data, offset);
        for (int i = 0; i < numGrav; i++) {
            m_gravity.push_back({readInt(data, offset)});
        }
        
        int numRise = readInt(data, offset);
        for (int i = 0; i < numRise; i++) {
            int start = readInt(data, offset);
            int end = readInt(data, offset);
            m_rising.push_back({start, end});
        }
        
        int numFall = readInt(data, offset);
        for (int i = 0; i < numFall; i++) {
            int start = readInt(data, offset);
            int end = readInt(data, offset);
            m_falling.push_back({start, end});
        }
        
        m_loaded = true;
    }
    
    int getBlockCount() { return m_blocks.size(); }
    BlockObject* getBlockAtIndex(int i) { return &m_blocks[i]; }
    int getBackgroundCount() { return m_backgrounds.size(); }
    BackgroundChange* getBackgroundAtIndex(int i) { return &m_backgrounds[i]; }
    int getGravityCount() { return m_gravity.size(); }
    GravityChange* getGravAtIndex(int i) { return &m_gravity[i]; }
    int getRisingCount() { return m_rising.size(); }
    BlocksRise* getRisingAtIndex(int i) { return &m_rising[i]; }
    int getFallingCount() { return m_falling.size(); }
    BlocksFall* getFallingAtIndex(int i) { return &m_falling[i]; }
    int getEndPos() { return m_endPos; }
    bool getLoadedSuccessfully() { return m_loaded; }
};

std::string buildObjectString(Level inLevel) {
    gdObj tempGD;
    BlockObject* tempIG;
    bool isPit = false;
    std::string result = level_string_base;
    
    for (int i = 0; i < inLevel.getBlockCount(); i++) {
        tempIG = inLevel.getBlockAtIndex(i);
        
        switch(tempIG->objType) {
            case 0: tempGD.p1_id = "1"; break;
            case 1: tempGD.p1_id = "8"; break;
            case 2: tempGD.p1_id = "9"; isPit = true; break;
        }
        
        if(!isPit) {
            tempGD.p2_x = utils::numToString(tempIG->xPos - 135);
            tempGD.p3_y = utils::numToString(tempIG->yPos + 15);
            tempGD.p21_colorID = "0";
            tempGD.p24_zLayer = "0";
            result += "1," + tempGD.p1_id + ",2," + tempGD.p2_x + ",3," + tempGD.p3_y + ",21,0,24,0;";
        }
        else {
            int iterations = round((tempIG->yPos - tempIG->xPos)/30) + 1;
            int currentX = (tempIG->xPos - 135);
            for(int j = 0; j < iterations; j++)
            {
                result += "1,9,2," + utils::numToString(currentX) + ",3,0,21,0,24,0;";
                currentX += 30;
            }
        }
        isPit = false;
    }
    
    BackgroundChange* tempBC;
    gdColorTrigger tempCT;
    
    for (int i = 0; i < inLevel.getBackgroundCount(); i++) {
        tempBC = inLevel.getBackgroundAtIndex(i);
        switch(tempBC->colorID) {
            case 0: tempCT.p7_red = "63"; tempCT.p8_green = "184"; tempCT.p9_blue = "199"; break;
            case 1: tempCT.p7_red = "236"; tempCT.p8_green = "216"; tempCT.p9_blue = "50"; break;
            case 3: tempCT.p7_red = "178"; tempCT.p8_green = "38"; tempCT.p9_blue = "227"; break;
            case 4: tempCT.p7_red = "241"; tempCT.p8_green = "19"; tempCT.p9_blue = "242"; break;
            case 2: tempCT.p7_red = "83"; tempCT.p8_green = "255"; tempCT.p9_blue = "83"; break;
            case 5: tempCT.p7_red = "0"; tempCT.p8_green = "0"; tempCT.p9_blue = "0"; break;
        }
        std::string xPosStr = utils::numToString(tempBC->xPos + 165);
        result += "1,899,2," + xPosStr + ",3,3000,7," + tempCT.p7_red + ",8," + tempCT.p8_green + ",9," + tempCT.p9_blue + ",10,0.25,23,1000,155,1,35,1;";
        result += "1,899,2," + xPosStr + ",3,3030,7," + tempCT.p7_red + ",8," + tempCT.p8_green + ",9," + tempCT.p9_blue + ",10,0.25,23,1001,155,1,35,1;";
        result += "1,899,2," + xPosStr + ",3,3060,7," + tempCT.p7_red + ",8," + tempCT.p8_green + ",9," + tempCT.p9_blue + ",10,0.25,23,1009,155,1,35,1;";
    }
    
    GravityChange* tempGC;
    gdMirrorPortal tempMP;
    gdCameraObj tempCO;
    bool currentlyInverted = false;
    
    for (int i = 0; i < inLevel.getGravityCount(); i++) {
        tempGC = inLevel.getGravAtIndex(i);
        tempMP.objID = currentlyInverted ? "46" : "45";
        tempCO.rotation = currentlyInverted ? "0" : "180";
        currentlyInverted = !currentlyInverted;
        std::string xPosStr = utils::numToString(tempGC->xPos + 165);
        result += tempMP.base + tempMP.objID + tempMP.middle + xPosStr + tempMP.remainder + ";";
        result += tempCO.base + xPosStr + tempCO.middle + tempCO.rotation + ";";
    }
    
    BlocksRise* tempBR;
    gdBlocksRise tempGBR;
    for (int i = 0; i < inLevel.getRisingCount(); i++) {
        tempBR = inLevel.getRisingAtIndex(i);
        tempGBR.xpos = utils::numToString(tempBR->startX - 465);
        result += tempGBR.base + "23" + tempGBR.middle + tempGBR.xpos + tempGBR.remainder + ";";
        tempGBR.xpos = (tempBR->startX == tempBR->endX) ? utils::numToString(inLevel.getEndPos() - 495) : utils::numToString(tempBR->endX - 495);
        result += tempGBR.base + "1915" + tempGBR.middle + tempGBR.xpos + tempGBR.remainder + ";";
    }
    
    BlocksFall* tempBF;
    gdBlocksFall tempGBF;
    for (int i = 0; i < inLevel.getFallingCount(); i++) {
        tempBF = inLevel.getFallingAtIndex(i);
        tempGBF.xpos = utils::numToString(tempBF->startX - 135);
        result += tempGBF.base + "23" + tempGBF.middle + tempGBF.xpos + tempGBF.remainder + ";";
        tempGBF.xpos = (tempBF->startX == tempBF->endX) ? utils::numToString(inLevel.getEndPos() - 15) : utils::numToString(tempBF->endX - 15);
        result += tempGBF.base + "1915" + tempGBF.middle + tempGBF.xpos + tempGBF.remainder + ";";
    }
    
    return result;
}

static auto IMPORT_PICK_OPTIONS = file::FilePickOptions {
    std::nullopt,
    #ifndef GEODE_IS_IOS
    {
        {
            "Impossible Game Levels (.lvl)",
            { "*.lvl" }
        }
    }
    #else
    {
        
        {
            "Impossible Game Levels (.dat)",
            { "*.dat" }
        }
    }
    #endif
};

class $modify(ImportLayer, LevelBrowserLayer) {
    struct Fields {
        async::TaskHolder<Result<std::pair<std::filesystem::path, std::string>>> m_importTask;
    };
    
    static Result<std::string> processLevelFile(std::filesystem::path const& path) {
        Level igLevel(path);
        
        if (igLevel.getBlockCount() == 0 && igLevel.getBackgroundCount() == 0 && igLevel.getEndPos() == 3015) {
            return Err("This is most likely not a valid Impossible Game level file");
        }
        if (!igLevel.getLoadedSuccessfully()) {
            return Err("This is not a valid Impossible Game level!");
        }
        
        std::string innerLevelString = buildObjectString(igLevel);
        return Ok(ZipUtils::compressString(innerLevelString, false, 0));
    }
    
    void onImport() {
        m_fields->m_importTask.spawn(
            "Importing Impossible Game Level",
            [this]() -> arc::Future<Result<std::pair<std::filesystem::path, std::string>>> {
                #ifdef GEODE_IS_IOS
                auto mode = file::PickMode::OpenFile;
                #else
                auto mode = file::PickMode::OpenFolder;
                #endif
                
                auto pickResult = co_await file::pick(mode, IMPORT_PICK_OPTIONS);
                if (pickResult.isErr()) co_return Err(pickResult.unwrapErr());
                
                auto pathOpt = pickResult.unwrap();
                if (!pathOpt.has_value()) co_return Err("No selection was made");
                
                std::filesystem::path finalPath = pathOpt.value();
                
                #ifndef GEODE_IS_IOS
                if (std::filesystem::is_directory(finalPath)) {
                    auto dirName = utils::string::toLower(utils::string::pathToString(finalPath.filename()));
                    
                    if (dirName.size() >= 4 && dirName.substr(dirName.size() - 4) == ".lvl") {
                        
                        auto filesResult = file::readDirectory(finalPath);
                        if (filesResult.isErr()) {
                            co_return Err("Failed to read directory: " + filesResult.unwrapErr());
                        }
                        
                        auto files = filesResult.unwrap();
                        bool found = false;
                        
                        for (auto const& filePath : files) {
                            if (!std::filesystem::is_regular_file(filePath)) continue;
                            
                            auto ext = utils::string::toLower(utils::string::pathToString(filePath.extension()));
                            
                            if (ext == ".lvl" || ext == ".dat" || ext.empty()) {
                                finalPath = filePath;
                                found = true;
                                break;
                            }
                        }
                        
                        if (!found) co_return Err("No level files found in selected .lvl folder");
                    } else {
                        auto filesResult = file::readDirectory(finalPath);
                        
                        if (filesResult.isErr()) {
                            co_return Err("Failed to read directory: " + filesResult.unwrapErr());
                        }
                        
                        auto files = filesResult.unwrap();
                        bool found = false;
                        
                        for (auto const& filePath : files) {
                            if (!std::filesystem::is_regular_file(filePath)) continue;
                            
                            auto ext = utils::string::toLower(utils::string::pathToString(filePath.extension()));
                            
                            if (ext == ".lvl") {
                                finalPath = filePath;
                                found = true;
                                break;
                            }
                        }
                        
                        if (!found) co_return Err("The chosen folder was not a .lvl");
                    }
                }
                #endif
                
                co_return co_await async::runtime().spawnBlocking<Result<std::pair<std::filesystem::path, std::string>>>([path = finalPath]() -> Result<std::pair<std::filesystem::path, std::string>> {
                    auto result = processLevelFile(path);
                    if (result.isErr()) return Err(result.unwrapErr());
                    return Ok(std::make_pair(path, result.unwrap()));
                });
            }(),
            
            [](Result<std::pair<std::filesystem::path, std::string>> result) {
                if (result.isErr()) {
                    if (result.unwrapErr() != "No selection was made") {
                        FLAlertLayer::create("Import Error", result.unwrapErr(), "OK")->show();
                    }
                    return;
                }
                
                auto [path, levelString] = result.unwrap();
                
                std::string name;
                
                for (auto it = path.begin(); it != path.end(); ++it) {
                    auto part = utils::string::pathToString(*it);
                    if (part.size() >= 4 && utils::string::toLower(part).substr(part.size() - 4) == ".lvl") {
                        name = utils::string::pathToString(std::filesystem::path(part).stem());
                        break;
                    }
                }
                
                if (name.empty()) {
                    auto ext = utils::string::toLower(utils::string::pathToString(path.extension()));
                    if (ext == ".lvl") {
                        name = utils::string::pathToString(path.stem());
                    }
                }
                
                if (!name.empty()) {
                    std::string first = name.substr(0, 1);
                    utils::string::toUpperIP(first);
                    name = first + name.substr(1);
                } else {
                    name = "Impossible Game Import";
                }
                
                auto gdLevel = GJGameLevel::create();
                gdLevel->m_levelType = GJLevelType::Editor;
                gdLevel->m_levelString = levelString;
                gdLevel->m_levelName = name;
                
                LocalLevelManager::get()->m_localLevels->insertObject(gdLevel, 0);
                
                auto scene = CCScene::create();
                auto layer = LevelBrowserLayer::create(GJSearchObject::create(SearchType::MyLevels));
                scene->addChild(layer);
                CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(.5f, scene));
            }
        );
    }
    
    bool init(GJSearchObject* search) {
        if (!LevelBrowserLayer::init(search)) return false;
        
        if (search->m_searchType == SearchType::MyLevels || search->m_searchType == SearchType::MyLists) {
            auto btnMenu = this->getChildByID("new-level-menu");
            auto igImportBtn = CCMenuItemExt::createSpriteExtra(
                CircleButtonSprite::createWithSpriteFrameName("file.png"_spr, .85f, CircleBaseColor::Pink, CircleBaseSize::Big),
                [this](CCMenuItemSpriteExtra* btn) { onImport(); }
            );
            
            igImportBtn->setID("import-ig-level-button"_spr);
            
            // This one has an ID but no layout which is CRINGE
            if (search->m_searchType == SearchType::MyLists && search->m_searchIsOverlay) {
                btnMenu->addChildAtPosition(igImportBtn, Anchor::BottomLeft, ccp(0, 60), false);
            }
            else {
                btnMenu->addChild(igImportBtn);
                btnMenu->updateLayout();
            }
        }
        return true;
    }
};