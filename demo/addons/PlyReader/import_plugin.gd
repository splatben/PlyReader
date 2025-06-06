@tool
extends EditorImportPlugin

func _get_importer_name() -> String:
	return "ply_importer"

func _get_visible_name() -> String:
	return "PLY Importer"

func _get_recognized_extensions() -> PackedStringArray:
	return ["ply"]

func _get_save_extension() -> String:
	return "mesh"

func _get_resource_type() -> String:
	return "ArrayMesh"

func _get_priority() -> float:
	return 1.0

func _get_preset_count() -> int:
	return 0

func _get_import_order() -> int:
	return 0

func _get_import_options(path: String, preset: int) -> Array[Dictionary]:
	return []


func _import(source_file, save_path, options, platform_variants, gen_files):
	var ply_reader = PlyReader.new()
	print(source_file)
	var arr_mesh := ply_reader.load_ply(source_file)
	if arr_mesh:
		ResourceSaver.save(arr_mesh,save_path +"."+ _get_save_extension())
		return OK
	else:
		printerr("Failed to import PLY file: " + source_file)
		return FAILED
