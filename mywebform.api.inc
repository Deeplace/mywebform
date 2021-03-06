<?php

/**
 * Load a webform
 *
 * @param mixed $id - the webform's identificator or technical name.
 * @param bool $reset - if TRUE, the function will ignore the cached webform object
 *                      and will force to request the database.
 * @return mixed - if the function succeeds the return value will be an object with
 *                 the form's data. Otherwise, FALSE will be returned.
 */
function mywebform_load($id = '', $reset = FALSE) {
  static $webforms = array();
  static $tech_names = array();

  // Convert tech_name to id
  $id = strtolower($id);
  if(!is_numeric($id) && !empty($tech_names[$id])) {
    $id = $tech_names[$id];
  }

  if(!isset($webforms[$id]) || $reset) {
    // Load webform
    if(is_numeric($id)) {
      $webform = db_query("SELECT * FROM {mywebforms} WHERE id = :id ", array(':id' => $id))->fetchObject();
    } else {
      $webform = db_query("SELECT * FROM {mywebforms} WHERE tech_name = :tech_name ", array(':tech_name' => $id))->fetchObject();
    }

    if($webform !== FALSE) {
      // Unserialize auxiliar values
      $webform->aux = @unserialize($webform->aux);
      if(empty($webform->aux)) $webform->aux = array();

      // Load files
      $webform->files = array();
      $result = db_query("SELECT * FROM {mywebform_files} WHERE mywebform_id = :mywebform_id ", array(':mywebform_id' => $webform->id));

      foreach($result as $data) {
        $webform->files[$data->type][] = $data;
      }

      // Load fields
      $webform->fields = array();
      $result = db_query("SELECT * FROM {mywebform_fields} WHERE mywebform_id = :mywebform_id AND status = :status ORDER BY id ", array(':mywebform_id' => $webform->id, ':status' => 1));

      foreach($result as $data) {
        $webform->fields[$data->name] = $data;
      }

      // Cache object
      $id = $webform->id;
      $webforms[$id] = $webform;
      $tech_names[strtolower($webform->tech_name)] = $id;
    } else {
      $webforms[$id] = FALSE;
    }
  }

  return $webforms[$id];
}

/**
 * Save a webform
 *
 * @param object $obj - the webform's data.
 * @return mixed - if the function succeeds, it will return TRUE. Otherwise, FALSE will be returned.
 */
function mywebform_save(&$obj) {
  if(empty($obj->aux)) $obj->aux = array();
  $obj->aux = serialize($obj->aux);
  if(empty($obj->html_template_view)) {
    $obj->html_template_view = '';
  }
  if(isset($obj->id)) {
    $result = drupal_write_record('mywebforms', $obj, 'id');
  } else {
    $obj->created = time();
    $result = drupal_write_record('mywebforms', $obj);
  }

  if($result) {
    foreach($obj->files as $type => $collection) {
      foreach($collection as $i => $file) {
        if(empty($file->fid)) {
          if(isset($file->id)) {
            db_query("DELETE FROM {mywebform_files} WHERE id = :id ", array(':id' => $file->id));
          }
        } else {
          $obj->files[$type][$i]->mywebform_id = $obj->id;
          $obj->files[$type][$i]->type = $type;
          if(isset($obj->files[$type][$i]->id)) {
            drupal_write_record('mywebform_files', $obj->files[$type][$i], 'id');
          } else {
            drupal_write_record('mywebform_files', $obj->files[$type][$i]);
          }
        }
      }
    }
  }

  return $result;
}

/**
 * Loads a single field
 *
 * @param int $id - the field's identificator
 * @param bool $reset - if TRUE, the function will ignore the cached field object
 *                      and will force to request the database.
 * @return mixed - if the function succeeds the return value will be an object with
 *                 the field's data. Otherwise, FALSE will be returned.
 */
function mywebform_field_load($id = '', $reset = FALSE) {
  static $fields = array();

  if(!isset($fields[$id]) || $reset) {
    $fields[$id] = db_query("SELECT * FROM {mywebform_fields} WHERE id = :id ", array(':id' => $id))->fetchObject();
  }

  return $fields[$id];
}

/**
 * Gets the path of the storage directory for webforms' files
 *
 * @return string - the base path of the storage
 */
function mywebform_files_dir() {
  //$path = file_create_path('webforms');
 // $path_to_folder = file_stream_wrapper_get_instance_by_uri('public://')->getDirectoryPath() .'/webforms';
  $path_to_folder = 'public://webforms';
  $return_path = 'webforms';
  $path = file_prepare_directory($path_to_folder, FILE_CREATE_DIRECTORY);
  $path = file_prepare_directory($path_to_folder, FILE_MODIFY_PERMISSIONS);
  if($path) {
    return $return_path;
  }
  else {
    return false;
  }
}

/**
 * Evaluates a string and obtain an associative array populated with the result elements
 *
 * @param string $str - the incoming string that may contain a SQL query, a PHP code or
 *                      group of key-values pairs separated by ;;, each pair in its own row.
 * @return array - an associative array with the values from the evaluated expression
 */
function mywebform_build_options($str, $default_item = '') {
  $options = array();

  if(isset($default_item)) {
    $options[''] = $default_item;
  }

  if(preg_match('/^.*SELECT.+FROM.+$/i', $str)) {
    $result = db_query($str);
    foreach($result as $data){
      $data = (Array)$data;
      $data = array_filter(array_map('trim', array_values($data)));
      $id = array_shift($data);
      $options[$id] = implode(' ', $data);
    }
  } elseif(preg_match("|<\?php(.*) \?>|i", trim(str_replace(array("\r", "\n"), array("", " "), $str)), $match)) {
    $options = eval($match[1]);
    if(empty($options)) {
      $options = array();
    }
  } else {
    $str = array_filter(array_map('trim', explode("\n", $str)));
    foreach($str as $line) {
      $line = trim($line);
      $arr = explode('|', $line, 2);
      if(count($arr) > 1) {
        $options[trim($arr[0])] = trim($arr[1]);
      } else {
        $options[$line] = $line;
      }
    }
  }
  return $options;
}

/**
 * Evaluates a string and obtain a JavaScript variable in string representation containing
 * the result
 *
 * @param string $str - the incoming string that may contain a SQL query, a PHP code or
 *                      group of key-values pairs separated by ;;, each pair in its own row.
 * @return string - a JavaScript variable in string representation
 */
function mywebform_build_container($container_id, $str) {
  $output = array();
  $i = 0;

  if(preg_match('/^.*SELECT.+FROM.+$/i', $str)) {
    $result = db_query($str);
    foreach($result as $data) {
      $data = (Array)$data;
      $arr = array();
      foreach($data as $key => $value) {
        $arr[] = sprintf("'%s' : %s", $key, drupal_json_output($value));
      }
      $output[] = sprintf("	'row_%d' : {%s}", $i++, implode(", ", $arr));
    }
  } elseif(preg_match("|<\?php(.*) \?>|i", trim(str_replace(array("\r", "\n"), array("", " "), $str)), $match)) {
    $result = eval($match[1]);
    foreach($result as $data) {
      $arr = array();
      foreach($data as $key => $value) {
        $arr[] = sprintf("'%s' : %s", $key, drupal_json_output($value));
      }
      $output[] = sprintf("	'row_%d' : {%s}", $i++, implode(", ", $arr));
    }
  } else {
    $result = array_filter(array_map('trim', explode("\n", $str)));
    foreach($result as $data) {
      $arr = explode('|', $data, 2);
      if(count($arr) > 1) {
        $output[] = sprintf("	'row_%d' : {'%s' : %s}", $i++, trim($arr[0]), trim($arr[1]));
      } else {
        $output[] = sprintf("	'row_%d' : {%s}", $i++, trim($arr[0]));
      }
    }
  }

  return $container_id . " = {\n" . implode(",\n", $output) . "\n};\n";
}

/**
 * Gets the predefined values to substitute as fields' values in webforms
 *
 * @param int $uid - user's identificator
 * @return array - an associative array with the predefined names as keys and their values
 *                 for the specified user
 */
function mywebform_predefined_values($uid = NULL) {
  $predefined = array();

  // Launch hook to get predefined values from other modules
  foreach (module_list() as $module) {
    $function = $module . '_fill_predefined_values';
    if (function_exists($function)) {
      $function($uid, $predefined);
    }
  }

  // Get predefined values from user input
  $user_defined = variable_get('mywebform_user_defined_values', '');
  $user_defined = array_filter(array_map('trim', explode("\n", $user_defined)));
  foreach($user_defined as $item) {
    $item = explode('|', $item, 2);
    $predefined[$item[0]] = $item[1];
  }

  return $predefined;
}

function mywebform_clear_xml($xml) {
  $i1 = strpos($xml, '<SignedDoc>');
  if($i1 !== FALSE) {
    $i1 += strlen('<SignedDoc>');
    $i2 = strpos($xml, '<Signature', $i1);
    if($i2 === FALSE) $i2 = strlen($xml);
    $xml = substr($xml, $i1, $i2 - $i1);
  }
  return $xml;
}

function mywebform_get_types($empty_element = NULL) {
  $options = array();

  if(isset($empty_element)) {
    $options[''] = $empty_element;
  }
  $result = db_query("SELECT tech_name, name FROM {mywebforms} WHERE status <> 0 ORDER BY name ");
  foreach($result as $data) {
    $options[strtolower($data->tech_name)] = $data->name;
  }

  return $options;
}

/**
 * Translate custom strings
 */
function mywebform_t($string, $args = array(), $langcode = NULL) {
  static $locales  = array();

  if(empty($string)) return '';

  if(empty($langcode)) {
    global $language;
    $langcode = $language->language;
  }

  $def_lang = language_default();
  if($langcode != $def_lang->language) {
    if(empty($locales[$langcode])) {
      $sql = "SELECT s.source AS source, t.translation FROM {locales_source} s
        LEFT JOIN {locales_target} t ON t.lid = s.lid
        WHERE s.textgroup = 'mywebform' AND (t.language = :language OR t.language IS NULL) ";

      $result = db_query($sql, array(':language' => $langcode));
      foreach($result as $data) {
        $locales[$langcode][$data->source] = empty($data->translation) ? $data->source : $data->translation;
      }
    }

    if(!isset($locales[$langcode][$string])) {
      $locales[$langcode][$string] = '';
      $obj = new stdClass();
      $obj->location = '';
      $obj->textgroup = 'mywebform';
      $obj->source = $string;
      $obj->version = VERSION;
      drupal_write_record('locales_source', $obj);
    } else {
      $string = $locales[$langcode][$string];
    }
  }

  if(!empty($args) && is_array($args)) {
    // Transform arguments before inserting them.
    foreach($args as $key => $value) {
      switch($key[0]) {
        case '@':
          // Escaped only.
          $args[$key] = check_plain($value);
          break;

        case '%':
        default:
          // Escaped and placeholder.
          $args[$key] = theme('placeholder', $value);
          break;

        case '!':
          // Pass-through.
      }
    }

    return strtr($string, $args);
  }

  return $string;
}