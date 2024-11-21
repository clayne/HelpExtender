#pragma once

namespace ObScript
{
	class Help
	{
	public:
		static void Install()
		{
			if (const auto function = RE::SCRIPT_FUNCTION::LocateConsoleCommand("Help"sv))
			{
				if (detail::IsInModule(reinterpret_cast<std::uintptr_t>(function->executeFunction)))
				{
					static RE::SCRIPT_PARAMETER params[] = {
						{ "matchstring (optional)", RE::SCRIPT_PARAM_TYPE::kChar, true },
						{ "filter (optional)", RE::SCRIPT_PARAM_TYPE::kInt, true },
						{ "form type (optional)", RE::SCRIPT_PARAM_TYPE::kChar, true }
					};

					function->SetParameters(params);
					function->executeFunction = &Execute;

					SKSE::log::debug("Registered Help."sv);
				}
				else
				{
					SKSE::log::error("Failed to register Help, command was already overridden."sv);
				}
			}
			else
			{
				SKSE::log::error("Failed to find Help function."sv);
			}
		}

		static void ClearCellMap()
		{
			m_CellMap.clear();
		}

	private:
		class detail
		{
		public:
			static const char* GetFormEditorID(const RE::TESForm* a_form)
			{
				switch (a_form->GetFormType())
				{
					case RE::FormType::Keyword:
					case RE::FormType::LocationRefType:
					case RE::FormType::Action:
					case RE::FormType::MenuIcon:
					case RE::FormType::Global:
					case RE::FormType::HeadPart:
					case RE::FormType::Race:
					case RE::FormType::Sound:
					case RE::FormType::Script:
					case RE::FormType::Navigation:
					case RE::FormType::Cell:
					case RE::FormType::WorldSpace:
					case RE::FormType::Land:
					case RE::FormType::NavMesh:
					case RE::FormType::Dialogue:
					case RE::FormType::Quest:
					case RE::FormType::Idle:
					case RE::FormType::AnimatedObject:
					case RE::FormType::ImageAdapter:
					case RE::FormType::VoiceType:
					case RE::FormType::Ragdoll:
					case RE::FormType::DefaultObject:
					case RE::FormType::MusicType:
					case RE::FormType::StoryManagerBranchNode:
					case RE::FormType::StoryManagerQuestNode:
					case RE::FormType::StoryManagerEventNode:
					case RE::FormType::SoundRecord:
						break;

					default:
						{
							auto hndl = REX::W32::GetModuleHandleA("po3_Tweaks");
							auto func = reinterpret_cast<const char* (*)(std::uint32_t)>(REX::W32::GetProcAddress(hndl, "GetFormEditorID"));
							if (func)
							{
								return func(a_form->formID);
							}
						}
				}

				return a_form->GetFormEditorID();
			}

			static bool IsInModule(std::uintptr_t a_ptr)
			{
				auto& mod = REL::Module::get();
				auto seg = mod.segment(REL::Segment::textx);
				auto end = seg.address() + seg.size();
				return (seg.address() < a_ptr) && (a_ptr < end);
			}

			static void SortFormArray(std::vector<RE::TESForm*>& a_forms)
			{
				std::sort(
					a_forms.begin(),
					a_forms.end(),
					[](const RE::TESForm* a_lhs, const RE::TESForm* a_rhs)
					{
						auto FormEnumString = RE::FORM_ENUM_STRING::GetFormEnumString();
						auto L_FORM = FormEnumString[a_lhs->formType.underlying()].formCode;
						auto R_FORM = FormEnumString[a_rhs->formType.underlying()].formCode;
						if (L_FORM != R_FORM)
						{
							return L_FORM < R_FORM;
						}

						auto L_EDID = detail::GetFormEditorID(a_lhs);
						auto R_EDID = detail::GetFormEditorID(a_rhs);
						auto C_EDID = _stricmp(L_EDID, R_EDID);
						if (C_EDID != 0)
						{
							return C_EDID < 0;
						}

						return a_lhs->formID < a_rhs->formID;
					});
			}

			static bool strempty(const std::string_view& a_string)
			{
				return a_string.empty();
			}

			static bool strvicmp(const std::string_view& a_string, const std::string_view& a_matchString)
			{
				auto it = std::search(
					a_string.begin(),
					a_string.end(),
					a_matchString.begin(),
					a_matchString.end(),
					[](char a_s, char a_m)
					{ return std::tolower(a_s) == std::tolower(a_m); });
				return it != a_string.end();
			}
		};

		static bool Execute(
			const RE::SCRIPT_PARAMETER* a_paramInfo,
			RE::SCRIPT_FUNCTION::ScriptData* a_scriptData,
			RE::TESObjectREFR* a_thisObj,
			RE::TESObjectREFR* a_containingObj,
			RE::Script* a_scriptObj,
			RE::ScriptLocals* a_locals,
			double&,
			std::uint32_t& a_opcodeOffsetPtr)
		{
			m_MatchString[0] = '\0';
			m_FormTFilter[0] = '\0';
			m_ExtCellHeader = false;
			m_Forms.clear();

			std::uint32_t idx{ 0 };
			RE::Script::ParseParameters(
				a_paramInfo,
				a_scriptData,
				a_opcodeOffsetPtr,
				a_thisObj,
				a_containingObj,
				a_scriptObj,
				a_locals,
				m_MatchString,
				&idx,
				m_FormTFilter);

			if (detail::strempty(m_MatchString))
			{
				ShowHelp_Usage();
				return true;
			}

			switch (idx)
			{
				case 0:
					ShowHelp_Funcs();
					ShowHelp_Settings();
					ShowHelp_Globs();
					ShowHelp_Forms();
					ShowHelp_Usage();
					break;

				case 1:
					ShowHelp_Funcs();
					ShowHelp_Usage();
					break;

				case 2:
					ShowHelp_Settings();
					ShowHelp_Usage();
					break;

				case 3:
					ShowHelp_Globs();
					ShowHelp_Usage();
					break;

				case 4:
					ShowHelp_Forms();
					ShowHelp_Usage();
					break;

				default:
					ShowHelp_Usage();
					break;
			}

			return true;
		}

		static void ShowHelp_Funcs_Print(RE::SCRIPT_FUNCTION a_function)
		{
			auto name = a_function.functionName;
			auto nick = a_function.shortName;
			auto help = a_function.helpString;

			if (!name)
			{
				return;
			}

			if (detail::strempty(name))
			{
				return;
			}

			std::string match;
			if (help && !detail::strempty(help))
			{
				if (nick && !detail::strempty(nick))
				{
					match = std::format("{:s} ({:s}) -> {:s}"sv, name, nick, help);
				}
				else
				{
					match = std::format("{:s} -> {:s}"sv, name, help);
				}
			}
			else
			{
				if (nick && !detail::strempty(nick))
				{
					match = std::format("{:s} ({:s})"sv, name, nick);
				}
				else
				{
					match = std::format("{:s}"sv, name);
				}
			}

			RE::ConsoleLog::GetSingleton()->Print(match.data());
		}

		static void ShowHelp_Funcs_Match(RE::SCRIPT_FUNCTION a_function)
		{
			if (detail::strempty(m_MatchString))
			{
				ShowHelp_Funcs_Print(a_function);
				return;
			}

			auto name = a_function.functionName;
			auto nick = a_function.shortName;
			auto help = a_function.helpString;

			if ((name && detail::strvicmp(name, m_MatchString)) || (nick && detail::strvicmp(nick, m_MatchString)) || (help && detail::strvicmp(help, m_MatchString)))
			{
				ShowHelp_Funcs_Print(a_function);
			}
		}

		static void ShowHelp_Funcs()
		{
			RE::ConsoleLog::GetSingleton()->Print("----CONSOLE COMMANDS--------------------");
			auto consoleCommands = RE::SCRIPT_FUNCTION::GetFirstConsoleCommand();
			for (std::uint16_t i = 0; i < RE::SCRIPT_FUNCTION::Commands::kConsoleCommandsEnd; ++i)
			{
				ShowHelp_Funcs_Match(consoleCommands[i]);
			}

			RE::ConsoleLog::GetSingleton()->Print("----SCRIPT FUNCTIONS--------------------");
			auto scriptCommands = RE::SCRIPT_FUNCTION::GetFirstScriptCommand();
			for (std::uint16_t i = 0; i < RE::SCRIPT_FUNCTION::Commands::kScriptCommandsEnd; ++i)
			{
				ShowHelp_Funcs_Match(scriptCommands[i]);
			}
		}

		static void ShowHelp_Settings_Print(RE::Setting* a_setting)
		{
			std::string match;
			switch (a_setting->GetType())
			{
				case RE::Setting::Type::kBool:
					match = std::format(
						"{:s} = {:s}",
						a_setting->GetName(),
						a_setting->GetBool());
					break;

				case RE::Setting::Type::kFloat:
					match = std::format(
						"{:s} = {:0.2f}"sv,
						a_setting->GetName(),
						a_setting->GetFloat());
					break;

				case RE::Setting::Type::kSignedInteger:
					match = std::format(
						"{:s} = {:d}"sv,
						a_setting->GetName(),
						a_setting->GetSInt());
					break;

				case RE::Setting::Type::kColor:
					{
						auto color = a_setting->GetColor();
						match = std::format(
							"{:s} = R:{:d} G:{:d} B:{:d} A:{:d}"sv,
							a_setting->GetName(),
							color.red,
							color.green,
							color.blue,
							color.alpha);
					}
					break;

				case RE::Setting::Type::kString:
					match = std::format(
						"{:s} = {:s}"sv,
						a_setting->GetName(),
						a_setting->GetString());
					break;

				case RE::Setting::Type::kUnsignedInteger:
					match = std::format(
						"{:s} = {:d}"sv,
						a_setting->GetName(),
						a_setting->GetUInt());
					break;

				default:
					match = std::format(
						"{:s} = <UNKNOWN>"sv,
						a_setting->GetName());
					break;
			}

			RE::ConsoleLog::GetSingleton()->Print(match.data());
		}

		static void ShowHelp_Settings_Match(RE::Setting* a_setting)
		{
			if (detail::strempty(m_MatchString))
			{
				ShowHelp_Settings_Print(a_setting);
				return;
			}

			auto name = a_setting->name;
			if (name && detail::strvicmp(name, m_MatchString))
			{
				ShowHelp_Settings_Print(a_setting);
			}
		}

		static void ShowHelp_Settings()
		{
			RE::ConsoleLog::GetSingleton()->Print("----GAME SETTINGS-----------------------");
			if (auto GameSettingCollection = RE::GameSettingCollection::GetSingleton())
			{
				for (auto& iter : GameSettingCollection->settings)
				{
					ShowHelp_Settings_Match(iter.second);
				}
			}

			RE::ConsoleLog::GetSingleton()->Print("----INI SETTINGS------------------------");
			if (auto INISettingCollection = RE::INISettingCollection::GetSingleton())
			{
				if (auto INIPrefSettingCollection = RE::INIPrefSettingCollection::GetSingleton())
				{
					for (auto& iter : INISettingCollection->settings)
					{
						auto setting = INIPrefSettingCollection->GetSetting(iter->name);
						ShowHelp_Settings_Match(setting ? setting : iter);
					}
				}
			}
		}

		static void ShowHelp_Globs_Print(RE::TESGlobal* a_global)
		{
			auto edid = a_global->GetFormEditorID();
			if (!edid)
			{
				return;
			}

			if (detail::strempty(edid))
			{
				return;
			}

			auto match = std::format("{:s} = {:0.2f}"sv, edid, a_global->value);
			RE::ConsoleLog::GetSingleton()->Print(match.data());
		}

		static void ShowHelp_Globs_Match(RE::TESGlobal* a_global)
		{
			if (detail::strempty(m_MatchString))
			{
				ShowHelp_Globs_Print(a_global);
				return;
			}

			auto edid = a_global->GetFormEditorID();
			if (edid && detail::strvicmp(edid, m_MatchString))
			{
				ShowHelp_Globs_Print(a_global);
			}
		}

		static void ShowHelp_Globs()
		{
			RE::ConsoleLog::GetSingleton()->Print("----GLOBAL VARIABLES--------------------");
			if (auto TESDataHandler = RE::TESDataHandler::GetSingleton())
			{
				for (auto& iter : TESDataHandler->GetFormArray<RE::TESGlobal>())
				{
					ShowHelp_Globs_Match(iter);
				}
			}
		}

		static void ShowHelp_Forms_Print()
		{
			detail::SortFormArray(m_Forms);

			auto FormEnumString = RE::FORM_ENUM_STRING::GetFormEnumString();
			for (auto& iter : m_Forms)
			{
				auto form = FormEnumString[iter->formType.underlying()].formString;
				auto edid = detail::GetFormEditorID(iter);
				auto name = iter->GetName();

				auto match = std::format(
					"{:s}: {:s} ({:08X}) '{:s}'"sv,
					form,
					edid,
					iter->GetFormID(),
					name);
				RE::ConsoleLog::GetSingleton()->Print(match.data());
			}
		}

		static void ShowHelp_Forms_Match(RE::TESForm* a_form)
		{
			switch (a_form->GetFormType())
			{
				case RE::FormType::Global:
					break;

				case RE::FormType::Cell:
					{
						auto cell = a_form->As<RE::TESObjectCELL>();
						if (cell && cell->IsExteriorCell())
						{
							break;
						}
					}

				default:
					{
						auto edid = detail::GetFormEditorID(a_form);
						auto name = a_form->GetName();

						if ((edid && detail::strvicmp(edid, m_MatchString)) ||
						    (name && detail::strvicmp(name, m_MatchString)))
						{
							m_Forms.emplace_back(a_form);
						}
					}
					break;
			}
		}

		static void ShowHelp_Forms()
		{
			RE::ConsoleLog::GetSingleton()->Print("----OTHER FORMS-------------------------");
			if (auto TESDataHandler = RE::TESDataHandler::GetSingleton())
			{
				auto formType = RE::StringToFormType(m_FormTFilter);
				if (formType == RE::FormType::Global)
				{
					return;
				}

				if (formType != RE::FormType::None &&
				    formType != RE::FormType::Cell)
				{
					auto& forms = TESDataHandler->formArrays[std::to_underlying(formType)];
					for (auto iter : forms)
					{
						ShowHelp_Forms_Match(iter);
					}
				}
				else
				{
					auto [forms, lock] = RE::TESForm::GetAllForms();
					for (auto& iter : *forms)
					{
						if (formType == RE::FormType::Cell && iter.second->formType != RE::FormType::Cell)
						{
							continue;
						}

						ShowHelp_Forms_Match(iter.second);
					}
				}

				if (m_Forms.size())
				{
					ShowHelp_Forms_Print();
				}

				if (formType == RE::FormType::None ||
				    formType == RE::FormType::Cell)
				{
					ShowHelp_Cells();
				}
			}
		}

		static void ShowHelp_Cells_Print(const std::string_view& a_edid, const std::string_view& a_fileName)
		{
			if (!m_ExtCellHeader)
			{
				RE::ConsoleLog::GetSingleton()->Print("----EXTERIOR CELLS----------------------");
				m_ExtCellHeader = true;
			}

			if (!detail::strempty(a_fileName))
			{
				auto match = std::format("{:s} CELL: {:s}"sv, a_fileName, a_edid);
				RE::ConsoleLog::GetSingleton()->Print(match.data());
			}
			else
			{
				auto match = std::format("CELL: {:s}"sv, a_edid);
				RE::ConsoleLog::GetSingleton()->Print(match.data());
			}
		}

		static void ShowHelp_Cells_Match(RE::TESFile* a_file)
		{
			if (!a_file->OpenTES(RE::NiFile::OpenMode::kReadOnly, false))
			{
				SKSE::log::warn("failed to open file: {:s}"sv, a_file->fileName);
				return;
			}

			do
			{
				if (a_file->currentform.form == 'LLEC')
				{
					char edid[512]{ '\0' };
					bool gotEDID{ false };

					std::uint16_t data{ 0 };
					bool gotDATA{ false };

					std::uint32_t cidx{ 0 };
					cidx += a_file->compileIndex << 24;
					cidx += a_file->smallFileCompileIndex << 12;

					do
					{
						switch (a_file->GetCurrentSubRecordType())
						{
							case 'DIDE':
								gotEDID = a_file->ReadData(edid, a_file->actualChunkSize);
								if (gotEDID && gotDATA && ((data & 1) == 0))
								{
									m_CellMap.insert_or_assign(std::make_pair(cidx, edid), a_file->fileName);
									continue;
								}
								break;

							case 'ATAD':
								gotDATA = a_file->ReadData(&data, a_file->actualChunkSize);
								if (gotEDID && gotDATA && ((data & 1) == 0))
								{
									m_CellMap.insert_or_assign(std::make_pair(cidx, edid), a_file->fileName);
									continue;
								}
								break;

							default:
								break;
						}
					}
					while (a_file->SeekNextSubrecord());
				}
			}
			while (a_file->SeekNextForm(true));

			if (!a_file->CloseTES(false))
			{
				SKSE::log::warn("failed to close file: {:s}"sv, a_file->fileName);
			}
		}

		static void ShowHelp_Cells_Build()
		{
			auto TESDataHandler = RE::TESDataHandler::GetSingleton();
			for (auto iter : TESDataHandler->compiledFileCollection.files)
			{
				ShowHelp_Cells_Match(iter);
			}

			for (auto iter : TESDataHandler->compiledFileCollection.smallFiles)
			{
				ShowHelp_Cells_Match(iter);
			}
		}

		static void ShowHelp_Cells()
		{
			if (m_CellMap.empty())
			{
				ShowHelp_Cells_Build();
			}

			for (auto& iter : m_CellMap)
			{
				if (detail::strvicmp(iter.first.second, m_MatchString))
				{
					ShowHelp_Cells_Print(iter.first.second, iter.second);
				}
			}
		}

		static void ShowHelp_Usage()
		{
			RE::ConsoleLog::GetSingleton()->Print("usage: help <matchstring> <filter> <form type>");
			RE::ConsoleLog::GetSingleton()->Print("filters: 0-all 1-functions, 2-settings, 3-globals, 4-other forms");
			RE::ConsoleLog::GetSingleton()->Print("form type is 4 characters and is ignored unless the filter is 4.");
		}

	protected:
		inline static char m_MatchString[512];
		inline static char m_FormTFilter[512];
		inline static bool m_ExtCellHeader{ false };

		inline static std::vector<RE::TESForm*> m_Forms;
		inline static std::map<std::pair<std::uint32_t, const std::string>, std::string_view> m_CellMap;
	};
}
