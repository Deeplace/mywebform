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

  drupal_json_output($matches);
}

function mywebform_data_page($hash = '') {
  $result = array();

  try {
    global $user;
    if(empty($_GET['grid'])) throw new Exception();
    $data = db_query("SELECT * FROM {mywebform_forms} WHERE hash = :hash AND uid = :uid AND created > :created",
      array(':hash' => $hash, ':uid' => $user->uid, ':created' => time() - 3600))->fetchObject();
    if($data === FALSE) throw new Exception();
    $data->data = @unserialize($data->data_serial);
    $fields = array();
    $grids = array();
    $webform = $data->data['webform'];
    $start = isset($_GET['start']) ? $_GET['start'] : 0;
    $offset = isset($_GET['offset']) ? $_GET['offset'] : 0;

    if(is_array($webform->fields)) {
      foreach($webform->fields as $field) {
        if($_GET['grid'] == $field->grid_name && is_array($field->value)) {
          $i = 0;
          foreach($field->value as $key => $val) {
            if($i >= $start) {
              if($i >= $start + $offset) break;

              $content = '';
              $info = '';

              switch($field->widget) {
                case 'select':
                  $options = mywebform_build_options($field->options);
                  if(!isset($options[$val])) {
                    foreach($options as $k => $v) {
                      if(is_array($v)) {
                        foreach($v as $kk => $vv) {
                          if(isset($options[$k][$kk])) {
                            $content = $vv;
                          }
                        }
                      }
                    }
                  }
                  else {
                    $content = (string) $options[$val];
                  }

                  if($field->class == 'document_type') {
                    $info = 'document_type';
                  }
                  break;
                case 'file':
                  @list($md5, $fid, $url) = explode(';', $val, 3);
                  $info = new stdClass();
                  $val = '';
                  if(!empty($fid)) {
                    $file = file_load($fid);
                    $arr = explode('.', $file->filename);
                    $info->ext = strtolower(end($arr));
                    $info->filelink = l($file->filename, 'getfile', array('query' => array('fid' => $file->fid)));
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

              if(!in_array($key, $grids)) {
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
  } catch(Exception $e) {
    $result = array('error' => t('No declaration found'));
  }
  exit(drupal_json_output($result));
}

function mywebform_getfile_page() {
  $fid = !empty($_GET['fid']) ? (int) $_GET['fid'] : 0;
  if(!$fid) exit();
  $file = file_load($fid);
  if(empty($file)) drupal_not_found();

  global $user;
  if($file->uid == $user->uid || user_access('administer document files')) {
    if(file_exists($file->uri)) {
      header('Content-Description: File Transfer');
      header('Content-Type: ' . $file->filemime);
      header('Content-Disposition: attachment; filename="' . $file->filename . '"');
      header('Content-Transfer-Encoding: binary');
      header('Expires: 0');
      header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
      header('Pragma: public');
      header('Content-Length: ' . $file->filesize);

      readfile(drupal_realpath($file->uri));
    }
    else {
      drupal_not_found();
    }
  }
  else {
    drupal_access_denied();
  }

  exit();
}






