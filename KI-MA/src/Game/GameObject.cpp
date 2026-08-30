#include "GameObject.h"

namespace Game {

	void GameObject::saveToFile(std::ofstream& file, bool saveState)
	{
		file.write((char*)&flags, sizeof(flags));
		file.write((char*)&id, sizeof(id));
		file.write((char*)&triggerGroup, sizeof(triggerGroup));
		file.write((char*)&position, sizeof(position));
		file.write((char*)&size, sizeof(size));

		uint32_t nameLength = objectName.size();
		file.write((char*)&nameLength, sizeof(nameLength));
		file.write(objectName.c_str(), nameLength);

		uint32_t textureNameLength = textureName.size();
		file.write((char*)&textureNameLength, sizeof(textureNameLength));
		file.write(textureName.c_str(), textureNameLength);

		file.write((char*)&repeatTexture, sizeof(repeatTexture));

		file.write((char*)&collider, sizeof(collider));
		file.write((char*)&physics.mass, sizeof(physics.mass));
		if (saveState) {
			file.write((char*)&physics.velocity, sizeof(physics.velocity));
			file.write((char*)&physics.acceleration, sizeof(physics.acceleration));
		}

		uint32_t triggerCount = triggers.size();
		file.write((char*)&triggerCount, sizeof(triggerCount));
		for (uint32_t i = 0; i < triggerCount; i++) {
			triggers[i]->saveToFile(file, saveState);
		}
	}

	void GameObject::loadFromFile(std::ifstream& file, bool saveState)
	{
		file.read((char*)&flags, sizeof(flags));
		file.read((char*)&id, sizeof(id));
		file.read((char*)&triggerGroup, sizeof(triggerGroup));
		file.read((char*)&position, sizeof(position));
		file.read((char*)&size, sizeof(size));

		uint32_t nameLength;
		file.read((char*)&nameLength, sizeof(nameLength));
		objectName.resize(nameLength);
		file.read((char*)objectName.data(), nameLength);

		uint32_t textureNameLength;
		file.read((char*)&textureNameLength, sizeof(textureNameLength));
		textureName.resize(textureNameLength);
		file.read((char*)textureName.data(), textureNameLength);

		file.read((char*)&repeatTexture, sizeof(repeatTexture));
		file.read((char*)&collider, sizeof(collider));
		file.read((char*)&physics.mass, sizeof(physics.mass));
		
		if (saveState) {
			file.read((char*)&physics.velocity, sizeof(physics.velocity));
			file.read((char*)&physics.acceleration, sizeof(physics.acceleration));
		}

		uint32_t triggerCount;
		file.read((char*)&triggerCount, sizeof(triggerCount));
		for (uint32_t i = 0; i < triggerCount; i++) {
			TriggerType type;
			file.read((char*)&type, sizeof(type));

			switch (type) {
			case TriggerType::CameraTrigger:
				triggers.push_back(std::make_shared<CameraTrigger>());
				break;
			case TriggerType::ObjectMoveTrigger:
				triggers.push_back(std::make_shared<ObjectMoveTrigger>());
				break;
			case TriggerType::ScoreTrigger:
				triggers.push_back(std::make_shared<ScoreTrigger>());
				break;
			case TriggerType::FinishTrigger:
				triggers.push_back(std::make_shared<FinishTrigger>());
				break;
			case TriggerType::DamageTrigger:
				triggers.push_back(std::make_shared<DamageTrigger>());
				break;
			}

			triggers.back()->loadFromFile(file, type, saveState);
		}
	}
}
