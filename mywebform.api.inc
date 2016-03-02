<?php

/**
 * Return path to mywebform static resources
 *
 * @param string $tech_name - identifier of webform
 * @return string
 *  - Path to resource directory
 */
function mywebform_resource_path($tech_name) {
  $path = variable_get('mywebform_weforms_path', drupal_get_path('module', 'mywebform') . '/webforms');
  return $path . '/' . strtolower($tech_name);
}

function mywebform_field_access($webform, $field = NULL) {
  if (user_access('mywebform admin')) {
    if (empty($webform)) {
      $webform = mywebform_load($field->mywebform_id);
    }
    if (!in_array('fields', $webform->readonly)) {
      return TRUE;
    }
  }
  return FALSE;
}

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
  if (!is_numeric($id) && !empty($tech_names[$id])) {
    $id = $tech_names[$id];
  }

  if (!isset($webforms[$id]) || $reset) {
    // Load webform
    if (is_numeric($id)) {
      $webform = db_fetch_object(db_query("SELECT * FROM {mywebforms} WHERE id = %d", $id));
    }
    else {
      $webform = db_fetch_object(db_query("SELECT * FROM {mywebforms} WHERE tech_name = '%s'", $id));
    }

    if ($webform !== FALSE) {
      // Unserialize auxiliar values
      $webform->aux = @unserialize($webform->aux);
      if (empty($webform->aux)) {
        $webform->aux = array();
      }

      // Load stored content
      $webform->files = array();
      $webform->readonly = array();
      $path = mywebform_resource_path($webform->tech_name);
      if (file_exists($path) && is_dir($path)) {
        $webform->files = array();
        if (file_exists($path . '/autofields.txt')) {
          $webform->autofields = file_get_contents($path . '/autofields.txt');
          $webform->readonly[] = 'autofields';
        }
        if (file_exists($path . '/edit.tpl.html')) {
          $webform->html_template = file_get_contents($path . '/edit.tpl.html');
          $webform->readonly[] = 'html_template';
        }
        if (file_exists($path . '/preview.tpl.html')) {
          $webform->html_preview_template = file_get_contents($path . '/preview.tpl.html');
          $webform->readonly[] = 'html_preview_template';
        }
        if (file_exists($path . '/print.tpl.html')) {
          $webform->html_print_template = file_get_contents($path . '/print.tpl.html');
          $webform->readonly[] = 'html_print_template';
        }
        if (file_exists($path . '/script.js')) {
          $webform->files['js'][] = (object) array(
            'type' => 'js',
            'filename' => $path . '/script.js',
            'locked' => TRUE,
          );
          $webform->readonly[] = 'files_js';
        }
        if (file_exists($path . '/style.css')) {
          $webform->files['css'] = (object) array(
            'type' => 'css',
            'filename' => $path . '/style.css',
            'locked' => TRUE,
          );
          $webform->readonly[] = 'files_css';
        }
        if (file_exists($path . '/schema.xsd')) {
          $webform->files['css'] = (object) array(
            'type' => 'xsd',
            'filename' => $path . '/schema.xsd',
            'locked' => TRUE,
          );
          $webform->readonly[] = 'files_xsd';
        }

        $webform->files['doc'] = array();
        foreach (scandir($path) as $document) {
          if (!in_array($document, array('.', '..', 'autofields.txt', 'edit.tpl.html', 'preview.tpl.html', 'print.tpl.html', 'script.js', 'style.css', 'fields.json'))) {
            if (!in_array(pathinfo($document, PATHINFO_EXTENSION), array('js', 'css', 'html', 'txt', 'xml'))) {
              $webform->files['doc'][] = (object) array(
                'type' => 'doc',
                'filename' => $path . '/' . $document,
                'locked' => TRUE,
              );
            }
          }
        }

        if (!empty($webform->files['doc'])) {
          $webform->readonly[] = 'files_doc';
        }

        if (file_exists($path . '/fields.json')) {
          $fields = json_decode(file_get_contents($path . '/fields.json'), TRUE);
          if (!empty($fields)) {
            $webform->readonly[] = 'fields';
            $webform->fields = array();
            ksort($fields);
            foreach ($fields as $key => $value) {
              $value['id'] = 0;
              $value['mywebform_id'] = $webform->id;
              $value['created'] = $_SERVER['REQUEST_TIME'];
              $webform->fields[$key] = (object) $value;
            }
          }
        }
      }

      // Load files
      $result = db_query("SELECT * FROM {mywebform_files} WHERE mywebform_id = %d", $webform->id);
      while ($data = db_fetch_object($result)) {
        if (!in_array('files_' . $data->type, $webform->readonly)) {
          $webform->files[$data->type][] = $data;
        }
      }

      // Load fields
      if (!in_array('fields', $webform->readonly)) {
        $webform->fields = array();
        $result = db_query("SELECT * FROM {mywebform_fields} WHERE mywebform_id = %d ORDER BY name", $webform->id);
        while ($data = db_fetch_object($result)) {
          $webform->fields[$data->name] = $data;
          $webform->fields[$data->name]->aux = @unserialize($webform->fields[$data->name]->aux);
          if (empty($webform->fields[$data->name]->aux)) {
            $webform->fields[$data->name]->aux = array();
          }
        }
      }

      // Cache object
      $id = $webform->id;
      $webforms[$id] = $webform;
      $tech_names[strtolower($webform->tech_name)] = $id;
    }
    else {
      $webforms[$id] = FALSE;
    }
  }

  if (empty($webforms[$id])) {
    return $webforms[$id];
  }

  $obj = clone($webforms[$id]);
  foreach ($obj->fields as $key => $value) {
    $obj->fields[$key] = clone($value);
    if (!empty($obj->fields[$key]->value)) {
      unset($obj->fields[$key]->value);
    }
  }

  return $obj;
}

/**
 * Save a webform
 *
 * @param object $obj - the webform's data.
 * @return mixed - if the function succeeds, it will return TRUE. Otherwise, FALSE will be returned.
 */
function mywebform_save(&$obj) {
  if (empty($obj->aux)) {
    $obj->aux = array();
  }

  $obj->aux = serialize($obj->aux);

  if ($obj->id) {
    $result = drupal_write_record('mywebforms', $obj, 'id');
  }
  else {
    $obj->created = time();
    $result = drupal_write_record('mywebforms', $obj);
  }

  if ($result) {
    foreach ($obj->files as $type => $collection) {
      foreach ($collection as $i => $file) {
        if (empty($file->filename)) {
          if ($file->id) {
            db_query("DELETE FROM {mywebform_files} WHERE id = %d", $file->id);
          }
        }
        else {
          $obj->files[$type][$i]->mywebform_id = $obj->id;
          $obj->files[$type][$i]->type = $type;
          if ($obj->files[$type][$i]->id) {
            drupal_write_record('mywebform_files', $obj->files[$type][$i], 'id');
          }
          else {
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

  if (!isset($fields[$id]) || $reset) {
    $fields[$id] = db_fetch_object(db_query("SELECT * FROM {mywebform_fields} WHERE id = %d", $id));

    $fields[$id]->aux = @unserialize($fields[$id]->aux);
    if (empty($fields[$id]->aux)) {
      $fields[$id]->aux = array();
    }
  }
  return $fields[$id];
}

/**
 * Gets the path of the storage directory for webforms' files
 *
 * @return string - the base path of the storage
 */
function mywebform_files_dir() {
  $path = file_create_path('webforms');
  file_check_directory($path, FILE_CREATE_DIRECTORY);
  return $path;
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

  if (isset($default_item)) {
    $options[''] = $default_item;
  }

  if (preg_match('/^.*SELECT.+FROM.+$/i', $str)) {
    $result = db_query($str);
    while ($data = db_fetch_array($result)) {
      $data = array_filter(array_map('trim', array_values($data)));
      $id = array_shift($data);
      $options[$id] = implode(' ', $data);
    }
  }
  elseif(preg_match("|<\?php(.*) \?>|i", trim(str_replace(array("\r", "\n"), array("", " "), $str)), $match)) {
    $options = eval($match[1]);
    if (empty($options)) {
      $options = array();
    }
  }
  else {
    $str = array_filter(array_map('trim', explode("\n", $str)));
    foreach ($str as $line) {
      $line = trim($line);
      $arr = explode(';;', $line, 2);
      if (count($arr) > 1) {
        $options[trim($arr[0])] = trim($arr[1]);
      }
      else {
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

  if (preg_match('/^.*SELECT.+FROM.+$/i', $str)) {
    $result = db_query($str);
    while ($data = db_fetch_array($result)) {
      $arr = array();
      foreach ($data as $key => $value) {
        $arr[] = sprintf("'%s' : %s", $key, drupal_to_js($value));
      }
      $output[] = sprintf("  'row_%d' : {%s}", $i++, implode(", ", $arr));
    }
  }
  elseif(preg_match("|<\?php(.*) \?>|i", trim(str_replace(array("\r", "\n"), array("", " "), $str)), $match)) {
    $result = eval($match[1]);
    foreach ($result as $data) {
      $arr = array();
      foreach ($data as $key => $value) {
        $arr[] = sprintf("'%s' : %s", $key, drupal_to_js($value));
      }
      $output[] = sprintf("  'row_%d' : {%s}", $i++, implode(", ", $arr));
    }
  }
  else {
    $result = array_filter(array_map('trim', explode("\n", $str)));
    foreach ($result as $data) {
      $arr = explode(';;', $data, 2);
      if (count($arr) > 1) {
        $output[] = sprintf("  'row_%d' : {'%s' : %s}", $i++, trim($arr[0]), trim($arr[1]));
      }
      else {
        $output[] = sprintf("  'row_%d' : {%s}", $i++, trim($arr[0]));
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
function mywebform_predefined_values($uid = NULL, $webform_tech_name = NULL, $empty_fields = array()) {
  $predefined = array();

  // Launch hook to get predefined values from other modules
  foreach (module_list() as $module) {
    $function = $module . '_fill_predefined_values';
    if (function_exists($function)) {
      $function($uid, $predefined, $webform_tech_name, $empty_fields);
    }
  }

  // Get predefined values from user input
  $user_defined = variable_get('mywebform_user_defined_values', '');
  $user_defined = array_filter(array_map('trim', explode("\n", $user_defined)));
  foreach ($user_defined as $item) {
    $item = explode(';;', $item, 2);
    $predefined[$item[0]] = $item[1];
  }

  return $predefined;
}

function mywebform_clear_xml($xml) {
  $i1 = strpos($xml, '<SignedDoc>');
  if ($i1 !== FALSE) {
    $i1 += strlen('<SignedDoc>');
    $i2 = strpos($xml, '<Signature', $i1);

    if ($i2 === FALSE) {
      $i2 = strlen($xml);
    }

    $xml = substr($xml, $i1, $i2 - $i1);
  }

  return $xml;
}

function mywebform_get_types($empty_element = NULL) {
  $options = array();

  if (isset($empty_element)) {
    $options[''] = $empty_element;
  }

  $result = db_query("SELECT tech_name, name FROM {mywebforms} WHERE status <> 0 ORDER BY name");
  while ($data = db_fetch_object($result)) {
    $options[strtolower($data->tech_name)] = $data->name;
  }

  return $options;
}

/**
 * Translate custom strings
 */
function mywebform_t($string, $args = array(), $langcode = NULL) {
  static $locales = array();

  if (empty($string)) {
    return '';
  }

  if (empty($langcode)) {
    global $language;
    $langcode = $language->language;
  }

  $def_lang = language_default();
  if ($langcode != $def_lang->language) {
    if (!isset($locales[$langcode])) {
      $locales[$langcode] = array();
      $sql = "SELECT s.source AS source, t.translation FROM {locales_source} s
        LEFT JOIN {locales_target} t ON t.lid = s.lid
        WHERE s.textgroup = 'mywebform' AND (t.language = '%s' OR t.language IS NULL)";

      $result = db_query($sql, $langcode);
      while ($data = db_fetch_object($result)) {
        $locales[$langcode][$data->source] = empty($data->translation) ? $data->source : $data->translation;
      }
    }

    if (empty($locales[$langcode][$string])) {
      if (!db_result(db_query("SELECT 1 FROM {locales_source} WHERE source = '%s' ", $string))) {
        $locales[$langcode][$string] = $string;
        $obj = new stdClass();
        $obj->location = '';
        $obj->textgroup = 'mywebform';
        $obj->source = $string;
        $obj->version = VERSION;
        drupal_write_record('locales_source', $obj);
      }
    }
    else {
      $string = $locales[$langcode][$string];
    }
  }

  if (!empty($args) && is_array($args)) {
    // Transform arguments before inserting them.
    foreach ($args as $key => $value) {
      switch ($key[0]) {
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


/**
 * Render numeric value as literal string, like one hundred and five for 105
 *
 * @param int $NR
 * @return string
 */
function spell($NR) {
  static $numbers=array("una", "doua", "trei", "patru", "cinci", "sase", "sapte", "opt", "noua");
  static $numbersex=array("un", "doi", "o", "una");
  static $overten=array("unsprezece", "doisprezece", "treisprezece", "paisprezece", "cincisprezece", "saisprezece", "saptesprezece", "optsprezece","nouaspreceze");
  static $tens = array("zece", "douazeci", "treizeci", "patruzeci", "cincizeci", "saizeci", "saptezeci", "optzeci","nouazeci");
  static $gr=array("mie", "mii", "suta", "sute", "milion", "milioane", "miliard", "miliarde", "trilion", "trilioane", "cvadrilion", "cvadrilioane");

  if ($NR == 0) {
    return 'zero';
  } elseif ($NR < 10) {
    return $numbers[$NR-1];
  } elseif ($NR < 100) {
    if ($NR < 20) {
      if ($NR == 10) return $tens[0];
      else return $overten[$NR-11];
    } else {
      $d = $NR % 10;
      if ($d == 0) return $tens[intval($NR/10)-1]." ";
      else return $tens[intval($NR/10)-1]." si ".spell($d)." ";
    }
  } elseif ($NR < 1000) {
    $t = intval($NR/100);
    $f = $NR%100;
    if ($t == 1) return "{$numbersex[3]} {$gr[2]} " . ($f ? spell($f) : '');
    else return $numbers[$t-1]." {$gr[3]} " . ($f ? spell($f) : '');
  } elseif ($NR < 1000000) {
    $t = intval($NR/1000);
    $x = intval($NR%1000);
    if ($t == 1) return $numbersex[3]." {$gr[0]} ".($x ? spell($x) : '');
    else return spell($t)." {$gr[1]} ".($x ? spell($x) : '');
  } elseif ($NR < 1000000000) {
    $t = intval($NR/1000000);
    $x = intval($NR%1000000);
    if ($t == 1) return "{$numbersex[0]} {$gr[4]} ".($x ? spell($x) : '');
    else return spell($t)." {$gr[5]} ".($x ? spell($x) : '');
  } elseif ($NR < 1000000000000) {
    $t = intval($NR/1000000000);
    $x = intval($NR%1000000000);
    if ($t == 1) return "{$numbersex[0]} {$gr[6]} ".($x ? spell($x) : '');
    else return spell($t)." {$gr[7]} ".($x ? spell($x) : '');
  } elseif ($NR < 1000000000000000) {
    $t = intval($NR/1000000000000);
    $x = intval($NR%1000000000000);
    if ($t == 1) return "{$numbersex[0]} {$gr[8]} ".($x ? spell($x) : '');
    else return spell($t)." {$gr[9]} ".($x ? spell($x) : '');
  }
}

function numberToString($NR, $PL="lei", $SG="leu", $PL2="bani", $SG2="ban") {
  static $numbersex = array("un", "doi", "o");

  $PT = explode(".", better_number_format($NR, 2, "."));
  if ($PT[0] == 1) $RETURN = $numbersex[0]." ".$SG;
  elseif ($PT[0]  == 2) $RETURN = $numbersex[1]." ".$PL;
  else $RETURN = spell($PT[0])." ".$PL;

  if ($PT[1])
  {
    $RETURN .= " ";
    if ($PT[1] == 1) $RETURN .= "01 ".$SG2;
    else $RETURN .= $PT[1]." ".$PL2;
  }

  return ucfirst($RETURN);
}

function better_number_format($number, $decimals = 2, $dec_point = '.' ) {
  $pointpos = strpos(strtr($number, ',', '.'), '.');
  if ($pointpos === FALSE) {
    return $number . $dec_point . str_repeat('0', $decimals);
  }else {
    return substr($number, 0, $pointpos) . $dec_point . str_pad(substr($number, $pointpos+1), $decimals, '0');
  }
}
