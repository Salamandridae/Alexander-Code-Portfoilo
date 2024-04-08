#include "day1.h"

Day1::Day1()
{
    std::ifstream myfile;
    myfile.open("../AdventOfCode2022/day1.txt");
    std::string myline;

    int calorieCount = 0;
    int goldCalorie = 0;
    int silverCalorie = 0;
    int bronzeCalorie = 0;

    if ( myfile.is_open() )
    {
        while ( myfile )
        {
            std::getline (myfile, myline);
            //std::cout << "An elf is counting his calories. He currently has " << calorieCount << "." << std::endl;
            if (myline == "newline")
            {
                //std::cout << "This elf has " << calorieCount << " calories." << std::endl;
                if (calorieCount > goldCalorie)
                {
                    bronzeCalorie = silverCalorie;
                    silverCalorie = goldCalorie;
                    goldCalorie = calorieCount;
                }
                else if (calorieCount > silverCalorie)
                {
                    bronzeCalorie = silverCalorie;
                    silverCalorie = calorieCount;
                }
                else if (calorieCount > bronzeCalorie)
                {
                    bronzeCalorie = calorieCount;
                }
                std::cout << "The current gold, silver and bronze are " << goldCalorie << ", " << silverCalorie << " and " << bronzeCalorie << std::endl;
                calorieCount = 0;
            }
            else
            {
                calorieCount = calorieCount + std::stoi(myline);
            }
            //std::cout << myline << '\n';
        }
    }
    else {
        std::cout << "Couldn't open file\n";
    }

    std::cout << "The gold, silver and bronze total count was " << goldCalorie + silverCalorie + bronzeCalorie << "." << std::endl;
}
