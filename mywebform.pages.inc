<?php

function mywebform_predefined_autocomplete_page($string = '') {
  $matches = array();
  if ($string) {
    $predefined = mywebform_predefined_values();
    foreach($predefined as $key => $value) {
      if(strpos($key, $string) !== FALSE) {
        $matches[$key] = $key;
      }
    }
  }
  
  drupal_json($matches);
}

function mywebform_data_page($hash = '') {
  $result = array();
  
  try {
    global $user;
    
    if (empty($_GET['grid'])) {
      throw new Exception();
    }
    
    $sql = "SELECT * FROM {mywebform_forms} WHERE hash = '%s' AND uid = %d AND created > %d";
    $data = db_fetch_object(db_query($sql, $hash, $user->uid, time() - 3600));
    if ($data === FALSE) {
      throw new Exception();
    }
    
    $data->data = @unserialize($data->data_serial);
    $fields = array();
    $grids = array();
    $webform = $data->data['webform'];
    $start = isset($_GET['start']) ? $_GET['start'] : 0;
    $offset = isset($_GET['offset']) ? $_GET['offset'] : 0;
    
    if (is_array($webform->fields)) {
      foreach ($webform->fields as $field) {
        if ($_GET['grid'] == $field->grid_name && is_array($field->value)) {
          $i = 0;
          foreach ($field->value as $key => $val) {
            if ($i >= $start) {
              if ($i >= $start + $offset) {
                break;
              }
              
              $content = '';
              $info = '';
              
              switch ($field->widget) {
                case 'select':
                  $options = mywebform_build_options($field->options);
                  $content = (string) $options[$val];
                  break;
                  
                case 'file':
                  list($md5, $fid, $url) = explode(';', $val, 3);
                  $info = new stdClass();
                  $val = '';
                  if (!empty($fid)) {
                    $file = db_fetch_object(db_query("SELECT * FROM {files} WHERE fid = %d", $fid));
                    $arr = explode('.', $file->filename);
                    $info->ext = strtolower(end($arr));
                    $info->filelink = l($file->filename, 'getfile', array('query' => 'fid=' . $file->fid));
                    $info->size = format_size($file->filesize);
                    $val = $file->fid;
                  }
                  break;
              }
              
              $fields[$field->name . '-' . $key] = (object) array(
                'type' => $field->widget,
                'value' => $val,
                'info' => $info,
                'content' => $content,
                'container' => (string) $field->container,
              );
              
              if (!in_array($key, $grids)) {
                $grids[] = $key;
              }
            }
            $i++;
          }
        }
      }
    }
    
    $result['fields'] = $fields;
    $result['grids'] = $grids;
    $result['grid'] = $_GET['grid'];
    $result['start'] = $start;
    $result['offset'] = $offset;
  }
  catch (Exception $e) {
    $result = array('error' => t('No declaration found'));
  }
  
  exit(drupal_to_js($result));
}

function mywebform_xsd_page($tech_name = '') {
  $tech_name = trim(str_replace('.xsd', '', $tech_name));
  $webform = mywebform_load($tech_name);
  
  $schema = array();
  foreach ($webform->fields as $name => $field) {
    if (!empty($field->xpath)) {
      $xpath = array_filter(array_map('trim', explode('/', $field->xpath)));
      $node = &$schema;
      foreach ($xpath as $item) {
        if (!isset($node[$item])) {
          $node[$item] = array();
        }
        if (!is_array($node[$item])) {
          $node[$item] = array('#mixed' => TRUE);
        }
        $node = &$node[$item];
      }
      $node = $name;
    }
  }
  
  drupal_set_header('Content-Type: text/xml');
  
  $output = '<?xml version="1.0" encoding="utf-8"?>';
  $output .= '<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">';

  $output .= '<xs:simpleType name="localDateType">
                <xs:restriction base="xs:string">
                  <xs:pattern value="((\d{1,2}[-|/|.]\d{1,2}[-|/|.]\d{2,4})|(\d{2,4}[-|/|.]\d{1,2}[-|/|.]\d{1,2})([T|:|-|.| ]\d{2,2}[:|.|-]\d{2,2}[:|.|-]\d{2,2})?)?"/>
                </xs:restriction>
              </xs:simpleType>';

  $output .= '<xs:simpleType name="localDecimalType">
                <xs:restriction base="xs:string">
                </xs:restriction>
              </xs:simpleType>';

  $output .= _mywebform_generate_xsd_node($webform, $schema);
  $output .= '</xs:schema>';
  
  exit($output);
}

function _mywebform_generate_xsd_node($webform, $schema, $name = '') {
  static $map, $grids;
  
  $output = '';
  
  if (!isset($map)) {
    $map = array(
      'numeric' => 'xs:decimal',
      'money' => 'xs:decimal',
      'float' => 'xs:decimal',
      'date' => 'localDateType',
    );
    $grids = array();
    foreach ($webform->fields as $field) {
      if (!empty($field->grid_name) && !in_array($field->grid_name, $grids)) {
        $grids[] = $field->grid_name;
      }
    }
  }
  
  $line = '';
  $is_grid = FALSE;
  if (!empty($name)) {
    if (strpos($name, '[')) {
      list($name, $line) = array_map('trim', explode('[', $name));
      $line = trim($line, ']');
      //$line = empty($line) ? 'line' : $line;
    }
    
    $is_grid = in_array($name, $grids);
  }
  
  if (is_array($schema)) {
    if (!empty($name)) {
      if ($is_grid) {
        $output .= '<xs:element name="' . $name . '">';
        $output .= '<xs:annotation/>';
        $output .= '<xs:complexType>';
        $output .= '<xs:sequence>';
      }
      elseif (!empty($line)) {
        $output .= '<xs:element name="' . $name . '" minOccurs="0" maxOccurs="unbounded">';
        $output .= '<xs:annotation/>';
        $output .= '<xs:complexType>';
        $output .= '<xs:all>';
      }
      else {
        $mixed = '';
        if (isset($schema['#mixed'])) {
          $mixed = ' mixed="true"';
        }
        $output .= '<xs:element name="' . $name . '">';
        $output .= '<xs:complexType' . $mixed . '>';
        $output .= '<xs:all>';
      }
    }
    
    foreach ($schema as $node => $value) {
      $output .= _mywebform_generate_xsd_node($webform, $value, $node);
    }
    
    if (!empty($name)) {
      if ($name == 'dec' || $name == 'doc') {
        $output .= '</xs:all>';
        $output .= '<xs:attribute name="TypeTechName" type="xs:string" use="required"/>';
        $output .= '<xs:attribute name="TypeName" type="xs:string" use="required"/>';
        $output .= '</xs:complexType>';
        $output .= '</xs:element>';
      }
      elseif ($is_grid) {
        $output .= '</xs:sequence>';
        $output .= '</xs:complexType>';
        $output .= '</xs:element>';
      }
      elseif (!empty($line)) {
        $output .= '</xs:all>';
        $output .= '<xs:attribute name="' . $line . '" type="xs:decimal" use="required"/>';
        $output .= '</xs:complexType>';
        $output .= '</xs:element>';
      }
      else {
        $output .= '</xs:all>';
        $output .= '</xs:complexType>';
        $output .= '</xs:element>';
      }
    }
  }
  elseif (!empty($name) && !empty($webform->fields[$schema])) {
    $field = $webform->fields[$schema];
    
    $attr = array(
      'name' => $name,
    );
    if (!empty($field->default_value) && strpos($field->default_value, '@') === FALSE) {
      $attr['default'] = $field->default_value;
    }
    $attr['minOccurs'] = $field->required ? 1 : ($field->ignore_empty ? 0 : 1);
    $attr['maxOccurs'] = 1;
    
    $output .= '<xs:element ' . drupal_attributes($attr) . '>';
    if (empty($field->title)) {
      $output .= '<xs:annotation />';
    }
    else {
      $output .= '<xs:annotation><xs:documentation>' . $field->title . '</xs:documentation></xs:annotation>';
    }
    $output .= '<xs:simpleType>';
    
    $type = empty($map[$field->type]) ? 'xs:string' : $map[$field->type];
    $type = $type == 'xs:decimal' && !$field->required ? 'localDecimalType' : $type;
    
    $output .= '<xs:restriction base="' . $type . '">';
    
    if ($type == 'xs:string') {
      if (!empty($field->maxlength)) {
        $output .= '<xs:maxLength value="' . $field->maxlength . '"/>';
      }
      
      if (!empty($field->pattern)) {
        $output .= '<xs:pattern value="' . $field->pattern . '"/>';
      }
      elseif ($field->type == 'period') {
        $output .= '<xs:pattern value="((L/(0[1-9]|1[0-2])/20[0-9]{2,2})|(T/0[1-4]/20[0-9]{2,2})|(S/0[1-2]/20[0-9]{2,2})|(A/20[0-9]{2,2}))"/>';
      }
    }

    switch ($type) {
      case 'xs:decimal':
        if ($field->type == 'money') {
          $output .= '<xs:fractionDigits value="2"/>';
        }
        elseif (!empty($field->decimal_length)) {
          $output .= '<xs:fractionDigits value="' . $field->decimal_length . '"/>';
        }
        
        $length = empty($field->maxlength) ? 17 : $field->maxlength;
        $output .= '<xs:totalDigits value="' . $length . '"/>';
        break;
      case 'localDecimalType':
        $decimal_length = 0;
        if ($field->type == 'money') {
          $decimal_length = 2;
        }
        elseif (!empty($field->decimal_length)) {
          $decimal_length = $field->decimal_length;
        }
        $max_length = empty($field->maxlength) ? 17 : $field->maxlength;
        $int_length = '0,' . ($max_length - $decimal_length);
        $decimal_length = $decimal_length < 1 ? '0,' : '0,' . $decimal_length;
        $output .= '<xs:pattern value="(\d{' . $int_length . '}(\.\d{' . $decimal_length . '})?)"/>';
        break;
    }
    
    $output .= '</xs:restriction>';
    
    $output .= '</xs:simpleType>';
    $output .= '</xs:element>';
  }
  
  return $output;
}
