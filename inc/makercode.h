#ifndef MAKERCODE_H
#define MAKERCODE_H

#include <unordered_map>
#include <string>

class Makercode {
public:
    // 构造函数，初始化字典
    Makercode() {
        // 在构造函数中初始化所有键值对
        publisher["01"] = "Nintendo";
        publisher["02"] = "Rocket Games";
        publisher["03"] = "Imagineer Zoom";
        publisher["04"] = "Gray Matter";
        publisher["05"] = "Zamuse";
        publisher["06"] = "Falcom";
        publisher["07"] = "Enix";
        publisher["08"] = "Capcom";
        publisher["09"] = "Hot B";
        publisher["0A"] = "Jaleco";
        publisher["13"] = "Electronic Arts Japan";
        publisher["18"] = "Hudson Entertainment";
        publisher["20"] = "Destination Software";
        publisher["36"] = "Codemasters";
        publisher["41"] = "Ubisoft";
        publisher["4A"] = "Gakken";
        publisher["4F"] = "Eidos";
        publisher["4Q"] = "Disney Interactive Studios";
        publisher["4Z"] = "Crave Entertainment";
        publisher["52"] = "Activision";
        publisher["54"] = "ROCKSTAR GAMES";
        publisher["5D"] = "Midway";
        publisher["5G"] = "Majesco Entertainment";
        publisher["64"] = "LucasArts Entertainment";
        publisher["69"] = "Electronic Arts Inc.";
        publisher["6K"] = "UFO Interactive";
        publisher["6V"] = "JoWooD Entertainment";
        publisher["70"] = "Atari";
        publisher["78"] = "THQ";
        publisher["7D"] = "Vivendi Universal Games";
        publisher["7J"] = "Zoo Digital Publishing Ltd";
        publisher["7N"] = "Empire Interactive";
        publisher["7U"] = "Ignition Entertainment";
        publisher["7V"] = "Summitsoft Entertainment";
        publisher["8J"] = "General Entertainment";
        publisher["8P"] = "SEGA";
        publisher["99"] = "Rising Star Games";
        publisher["A4"] = "Konami Digital Entertainment";
        publisher["AF"] = "Namco";
        publisher["B2"] = "Bandai";
        publisher["E9"] = "Natsume";
        publisher["EB"] = "Atlus";
        publisher["FH"] = "Foreign Media Games";
        publisher["FK"] = "The Game Factory";
        publisher["FP"] = "Mastiff";
        publisher["FQ"] = "iQue";
        publisher["FR"] = "dtp young";
        publisher["G9"] = "D3Publisher of America";
        publisher["GD"] = "SQUARE ENIX";
        publisher["GL"] = "gameloft";
        publisher["GN"] = "Oxygen Interactive";
        publisher["GR"] = "GSP";
        publisher["GT"] = "505 Games";
        publisher["GQ"] = "Engine Software";
        publisher["GY"] = "The Game Factory";
        publisher["H3"] = "Zen";
        publisher["H4"] = "SNK PLAYMORE";
        publisher["H6"] = "MYCOM";
        publisher["HC"] = "Plato";
        publisher["HF"] = "Level 5";
        publisher["HG"] = "Graffiti Entertainment";
        publisher["HM"] = "HMH - INTERACTIVE";
        publisher["HV"] = "bhv Software GmbH";
        publisher["LR"] = "Asylum Entertainment";
        publisher["KJ"] = "Gamebridge";
        publisher["KM"] = "Deep Silver";
        publisher["MJ"] = "MumboJumbo";
        publisher["MT"] = "Blast Entertainment";
        publisher["N6"] = "McDonalds";
        publisher["NK"] = "Neko Entertainment";
        publisher["NP"] = "Nobilis Publishing";
        publisher["PG"] = "Phoenix Games";
        publisher["PL"] = "Playlogic";
        publisher["SU"] = "Slitherine Software UK Ltd";
        publisher["SV"] = "SevenOne Intermedia GmbH";
        publisher["RM"] = "rondomedia";
        publisher["RT"] = "RTL Games";
        publisher["TK"] = "Tasuke";
        publisher["TR"] = "Tetris Online";
        publisher["TV"] = "Tivola Publishing";
        publisher["VP"] = "Virgin Play";
        publisher["WP"] = "White Park Bay";
        publisher["WR"] = "Warner Bros";
        publisher["XS"] = "Aksys Games";
    }

    // Find publisher name
    std::string findPublisher(const std::string& key) const {
        auto it = publisher.find(key);
        if (it != publisher.end()) {
            return it->second;
        }
        return "Unknown";
    }

private:
    // 存储字典的 unordered_map
    std::unordered_map<std::string, std::string> publisher;
};

#endif // MAKERCODE_H
