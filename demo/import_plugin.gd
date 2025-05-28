@tool
extends EditorImportPlugin

enum Presets { PRESET_DEFAULT }

func _get_importer_name() -> String:
	return "pointcloud importer"

func _get_visible_name() -> String:
	return "Point Cloud"

func _get_recognized_extensions() -> PackedStringArray:
	return ["ply"]

func _get_save_extension() -> String:
	return "mesh"

func _get_resource_type() -> String:
	return "ArrayMesh"

func _get_preset_count() -> int:
	return Presets.size()

func _get_preset_name(preset: int) -> String:
	match preset:
		Presets.PRESET_DEFAULT:
			return "Default"
		_:
			return "Unknown"

func _get_import_options(path: String, preset: int) -> Array[Dictionary]:
	match preset:
		Presets.PRESET_DEFAULT:
			return [
				{
					"name": "point_scale",
					"default_value": 0.2,
					"hint_string": "Scale for shader point size. Default is double of model scale factor."
				},
				{
					"name": "scale_factor",
					"default_value": 0.1,
					"hint_string": "Amount by which the coordinates in the file will be divided."
				}
			]
		_:
			return []

func _get_option_visibility(path: String, option: StringName, options: Dictionary) -> bool:
	return true

func _get_priority() -> float:
	return 1.0

func _import(source_file: String, save_path: String, options: Dictionary, platform_variants: Array, gen_files: Array) -> int:
	# Ouverture du fichier via FileAccess (Godot 4.x)
	
	# Sauvegarde du mesh dans le chemin de destination
	return ResourceSaver.save(array_mesh, "%s.%s" % [save_path, _get_save_extension()])
