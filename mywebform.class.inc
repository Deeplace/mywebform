<?php

class WebForm {
  public $data = array();
  public $errors = array();
  public $resultset = array();
  public $input = array();
  public $webform;
  public $revalidate = FALSE;
  public $extension = '';
  public $is_draft = 0;
  protected $xml;
  protected $submit_callback;
  protected $predefined_values;
  protected $hash = '';
  protected $document;

  public function __construct($webform_tech_name, $xml, $submit_callback = NULL, $author_uid = NULL, $document = NULL) {
    $this->webform = mywebform_load($webform_tech_name);

    if(empty($this->webform)) throw new WebFormException('Webform not found');
    $this->xml = $xml;
    $this->submit_callback = $submit_callback;
    $this->predefined_values = mywebform_predefined_values($author_uid);

    $this->document = $document;
    // Load values
    $this->LoadFieldsValues();
  }

  public function Render($preview = FALSE, $print_version = FALSE) {
    $valid = $preview ? TRUE : $this->Submit();

    drupal_add_css(drupal_get_path('module', 'mywebform') . '/mywebform.css');
    drupal_add_js(drupal_get_path('module', 'mywebform') . '/mywebform.js');
    drupal_add_js(drupal_get_path('module', 'mywebform') . '/common.js');

    $grids = $this->GetGridList();
    if(!empty($grids)) {
      $this->SaveToCache();
      drupal_add_js(drupal_get_path('module', 'mywebform') . '/grid.js');
    }

    // Set JavaScript variables
    $attr = array(
      'grids' => $grids,
      'load_url' => '',
      'revalidate' => !$valid,
      'print' => FALSE,
      'instantprint' => FALSE,
      'preview' => (bool) $preview,
      'userprofile' => url('userprofile'),
      'grid_timer' => 100,
      'edit' => FALSE
    );
    if(!empty($this->xml) && $this->hash) {
      $attr['load_url'] = url('mywebform/data/' . $this->hash);
    }

    if(!empty($this->xml)) {
      $attr['edit'] = TRUE;
    }
    if($preview) {
      $attr['print'] = $print_version;
      $attr['instantprint'] = empty($grids) && !empty($print_version);
    }
    drupal_add_js(array('mywebform' => $attr), 'setting');

    return $this->_RenderForm($preview);
  }

  public function OutputXml() {
    $post = array();

    foreach($this->webform->fields as $field) {
      if(empty($field->xpath)) continue;
      if(!empty($field->ignore_empty) && empty($field->value)) continue;

      if(preg_match_all("|\[([^]]*)\]|i", $field->xpath, $matches)) {
        if(is_array($field->value)) {
          foreach($field->value as $key => $value) {
            $tr = array();
            foreach($matches[1] as $attr) {
              $tr['[' . $attr . ']'] = ' ' . $attr . '="' . $key . '"';
            }
            $post[strtr($field->xpath, $tr)] = $value;
          }
        }
      } else {
        if(is_array($field->value)) {
          $post[$field->xpath] = trim(implode(' ', $field->value));
        } else {
          $post[$field->xpath] = $field->value;
        }
      }
    }

    $complex = array();
    foreach($post as $name => $value) {
      _mywebform_complex_set_value($complex, explode('/', $name), htmlspecialchars($value));
    }
    $complex['doc']['#attr'] = array('TypeName' => $this->webform->name);
    return '<?xml version="1.0" encoding="UTF-8" ?>' . "\n" . _mywebform_complex_to_xml($complex);
  }

  protected function Submit() {
    if(!empty($_POST) && $_POST['form_id'] == 'mywebform-edit-form') {
      $this->hash = $_POST['hash'];

    if($_POST['document_state'] != 'save') {
      $this->is_draft = 1;
    }
    else {
      $this->is_draft = 0;
      // Validate
      if(!$this->ValidateFields()) {
        return FALSE;
      }
    }

      // Remove old files
      foreach($this->webform->fields as $field) {
        if($field->widget == 'file') {
          if(is_array($field->old_value)) {
            foreach($field->old_value as $key => $val) {
              if(!empty($val) && $val != @$field->value[$key]) {
                @list($md5, $fid, $url) = @explode(';', $val, 3);
                _mywebform_delete_file($fid);
              }
            }
          } else {
            if(!empty($field->old_value) && $field->old_value != $field->value) {
              @list($md5, $fid, $url) = @explode(';', $field->old_value, 3);
              _mywebform_delete_file($fid);
            }
          }
        }
      }

      $this->errors = $_POST['has_errors'];

      // Export values to resultset
      foreach($this->webform->fields as $field) {
        if($field->export_result) {
          $this->resultset[$field->name] = $field->value;
        }
      }

      // Submit
      db_delete('mywebform_forms')
      ->condition('hash', $this->hash)
      ->execute();
      if(function_exists($this->submit_callback)) {
        $func = $this->submit_callback;
        $func($this);
      } else {
        drupal_set_message(t('Webform callback function does not exist'), 'error');
      }

      drupal_goto($_GET['q']);
    } else {
      if($this->revalidate) return FALSE;
    }

    return TRUE;
  }

  protected function SaveToCache() {
    if(empty($this->hash)) {
      global $user;

      $this->hash = md5(uniqid(mt_rand(), TRUE));

      $data = new stdClass();
      $data->hash = $this->hash;
      $data->uid = $user->uid;
      $data->data_serial = serialize(array('webform' => $this->webform, ));
      $data->created = time();

      drupal_write_record('mywebform_forms', $data);
    } else {
      db_update('mywebform_forms')
      ->fields(array('created' => time()))
      ->condition('hash', $this->hash)
      ->execute();
    }
  }

  protected function LoadFieldsValues() {
    $post = _mywebform_xml_to_post($this->xml, $this->webform->fields);
    $submit = !empty($_POST) && $_POST['form_id'] == 'mywebform-edit-form';

    // Set default values
    foreach($this->webform->fields as $name => $field) {
      $value = NULL;

      if($submit && !$field->disabled && !$field->readonly) {
        if($field->widget == 'checkbox') {
          if(isset($_POST[$name])) {
            if(is_array($_POST[$name])){
              foreach($_POST[$name] as $kk=>$vv) {
                $value[$kk] = $vv;
              }
            }
            else {
              $value = $_POST[$name];
            }
          }
          else {
            $value = 0;
          }
        } elseif($field->widget == 'file') {
          // Save current file to old_value property to farther remove it if a new
          // one is uploaded
          $this->webform->fields[$name]->old_value = (isset($post[$name])) ? $post[$name] : '';
          $validators = array();
          $validators['file_validate_name_length'] = array();
          if(!empty($this->webform->fields[$name]->upload_file_extensions)) {
            $validators['file_validate_extensions'] = array(str_replace(',' , ' ', $this->webform->fields[$name]->upload_file_extensions));
          }
          if(!empty($this->webform->fields[$name]->max_file_size)) {
            if(strpos($this->webform->fields[$name]->max_file_size, 'K', strlen($this->webform->fields[$name]->max_file_size -1))) {
              $size = substr($this->webform->fields[$name]->max_file_size, 0, strlen($this->webform->fields[$name]->max_file_size) - 1);
              $validators['file_validate_size'] = array($size * 1024);
            }
            elseif(strpos($this->webform->fields[$name]->max_file_size, 'M', strlen($this->webform->fields[$name]->max_file_size -1))) {
              $size = substr($this->webform->fields[$name]->max_file_size, 0, strlen($this->webform->fields[$name]->max_file_size) - 1);
              $validators['file_validate_size'] = array($size * 1024 * 1024);
            }
            else {
              $validators['file_validate_size'] = array(intval($this->webform->fields[$name]->max_file_size));
            }
          }

          $filepath = variable_get('mywebform_document_attachment_folder','private://attachments');

          file_prepare_directory($filepath, FILE_CREATE_DIRECTORY);
          file_prepare_directory($filepath, FILE_MODIFY_PERMISSIONS);

          $i = 1;

          if(@is_array($_FILES['files']['name'][$name . '-file']) ) {
            $temp_global_files = $_FILES;
            foreach($temp_global_files['files']['name'][$name . '-file'] as $k => $v) {
              $_FILES = array();
              $file = FALSE;
              if(!empty($v)) {
                $save_file_name = $name . '-file_' . $i++;
                $_FILES['files']['name'][$save_file_name] = $v;
                $_FILES['files']['type'][$save_file_name] = $temp_global_files['files']['type'][$name . '-file'][$k];
                $_FILES['files']['tmp_name'][$save_file_name] = $temp_global_files['files']['tmp_name'][$name . '-file'][$k];
                $_FILES['files']['error'][$save_file_name] = $temp_global_files['files']['error'][$name . '-file'][$k];
                $_FILES['files']['size'][$save_file_name] = $temp_global_files['files']['size'][$name . '-file'][$k];
                if(!empty($_FILES['files']['size'][$save_file_name])) {
                  $file = file_save_upload($save_file_name, $validators, $filepath);
                }
              }
              if($file !== FALSE) {
                $file->status = FILE_STATUS_PERMANENT;

                file_save($file);

                $md5 = md5_file(drupal_realpath($file->uri));
                $url = url('getfile', array('absolute' => TRUE,'query' => array('fid' => $file->fid)));
                $value[$k] = $md5 . ';' . $file->fid . ';' . $url;
              }
              else {
                if(empty($_POST[$name . '-remove'][$k])) {
                  $value[$k] = @$post[$name][$k];
                }
              }
            }
            $_FILES = $temp_global_files;
          }
          else {
            $file = FALSE;
            if(!empty($_FILES['files']['name'][$field->name . '-file'])) {
              $file = file_save_upload($field->name . '-file', $validators, $filepath);
            }
            if($file !== FALSE) {
              $file->status = FILE_STATUS_PERMANENT;

              file_save($file);

              $md5 = md5_file(drupal_realpath($file->uri));
              $url = url('getfile', array('absolute' => TRUE,'query' => array('fid' => $file->fid)));
              $value = $md5 . ';' . $file->fid . ';' . $url;
            }
            else {
              if(empty($_POST[$name . '-remove'])) {
                $value = @$post[$name];
              }
            }
          }
        } else {
          if(isset($_POST[$name])) {
            if(is_array($_POST[$name])) {
              $value = $_POST[$name];

              // Drop index 0 from array - it may be present due a JavaScript error
              // while validationg form before submision
              if(isset($value[0])) unset($value[0]);
            } else {
              $value = strlen((string) $_POST[$name]) ? $_POST[$name] : '';
            }
          }
        }
      } elseif(isset($post[$name])) {
        $value = $post[$name];
      }

      if($field->force_def_val || ($field->default_value && !isset($value))) {
        if(!($field->type === 'checkbox' && !empty($post))) {
          $value = strtr($field->default_value, $this->predefined_values);
        }
      }

      $this->webform->fields[$name]->value = $this->SanitizeValue($value, $field);

    }
  }

  protected function SanitizeValue($value, $field) {
    if(!strlen((string) $value)) return '';

    switch($field->type) {
      case 'float':
        if(is_array($value)) {
          foreach($value as $k => $v) {
            $value[$k] = round($v, $field->decimal_length);
          }
        } else {
          $value = round($value, $field->decimal_length);
        }
        break;
      case 'money':
        if(is_array($value)) {
          foreach($value as $k => $v) {
            $value[$k] = strlen($v) ? sprintf("%1.2f", $v) : '';
          }
        } else {
          $value = strlen($value) ? sprintf("%1.2f", $value) : '';
        }
        break;
      case 'text':
        if(is_array($value)) {
          foreach($value as $k => $v) {
            if(_mywebform_check_cyr_with_latin($v)) {
              $value[$k] = _mywebform_fix_cyr_with_latin($v);
            }
          }
        } else {
          if(_mywebform_check_cyr_with_latin($value)) {
            $value = _mywebform_fix_cyr_with_latin($value);
          }
        }
        break;
    }

    return $value;
  }

  protected function ValidateFields() {
    $error = FALSE;

    foreach($this->webform->fields as $name => $field) {
      $values = $field->value;
      if(!is_array($values)) $values = array($values);

      foreach($values as $key => $value) {
        // Check required
        if($field->required && empty($value)) {
          $error = TRUE;
        }

        // Check length
        if($field->minlength && strlen($value) < $field->minlength) {
          $error = TRUE;
        }
        if($field->maxlength && strlen($value) > $field->maxlength) {
          $error = TRUE;
        }

        // Sanitize values
        switch($field->type) {
          case 'money':
            $values[$key] = sprintf("%1.2f", str_replace(',', '.', $value));
            break;
          case 'float':
            if($field->decimal_length) {
              $values[$key] = round(str_replace(',', '.', $value), $field->decimal_length);
            }
            break;
          case 'custom':
            if(!empty($field->format)) {
              $values[$key] = sprintf($field->format, $value);
            }
            break;
        }

        // Check by pattern or sanitize values
        $pattern = '';
        switch($field->type) {
          case 'latintext':
            $pattern = "|^[a-z0-9,.-]*$|i";
            break;
          case 'numeric':
            $pattern = "|^([-][0-9]{1})?[0-9]*([.,,][0-9]+)?$|i";
            break;
          case 'date':
            $pattern = "|^([0-9]{2}[./]{1}[0-9]{2}[./]{1}[0-9]{4})?$|i";
            break;
          case 'period':
            $pattern = "/^((a)\/\d{4}$)|((t)\/[1234]\/\d{4}$)|((s|S)\/[12]\/\d{4}$)|((l|L)\/\d{1,2}\/\d{4})$/i";
            break;
        }

        if(!empty($pattern) && !preg_match($pattern, $value)) {
          $error = TRUE;
        }
      }
    }
    return !$error;
  }

  protected function GetGridList() {
    $grids = array();

    foreach($this->webform->fields as $name => $field) {
      if(!empty($field->grid_name) && empty($grids[$field->grid_name])) {
        $grids[$field->grid_name] = is_array($field->value) ? count($field->value) : 0;
      }
    }

    return $grids;
  }

  protected function _RenderForm($preview) {
    $output = '';

    if(!$preview) {
      global $language;

      // Add CSS and JS files
      drupal_add_js(drupal_get_path('module', 'mywebform') . '/validation.js');

      // JS Calendar
      drupal_add_css('misc/jscal2/css/jscal2.css');
      drupal_add_css('misc/jscal2/css/border-radius.css');
      drupal_add_css('misc/jscal2/css/steel/steel.css');
      drupal_add_js('misc/jscal2/js/jscal2.js');
      drupal_add_js('misc/jscal2/js/lang/' . $language->language . '.js');
    }

    if(isset($this->webform->files['css'])) {
      if(is_array($this->webform->files['css'])) {
        foreach($this->webform->files['css'] as $file) {
          $temp_file = file_load($file->fid);
          drupal_add_css($temp_file->uri);
        }
      }
    }
    if(isset($this->webform->files['js'])) {
      if(is_array($this->webform->files['js'])) {
        foreach($this->webform->files['js'] as $file) {
          $temp_file = file_load($file->fid);
          drupal_add_js($temp_file->uri);
        }
      }
    }

    // Add autofield JS functions
    $this->BuildAutofields();

    if($preview) {
      if(empty($_GET['print'])) {
        $output .= '<div class="forprint">' . l(t('Printer version'), $_GET['q'], array('query' => array('print' => '1'))) . '</div>';
      }
      $output .= '<div class="progressbar"><div class="current"><span>0 %</span></div><span class="comment">' . t('Loading...') . '</span></div>';
      $output .= '<div id="mywebform-content" class="preview">';
      $output .= $this->BuildFormHtml($preview);
      $output .= '</div>';
    } else {
      $form_attr = array(
        'id' => 'mywebform-edit-form',
        'method' => 'post',
        'enctype' => 'multipart/form-data',
        'accept-charset' => 'UTF-8',
        'action' => url($_GET['q']),
        'class' => array('uniform')
      );
      $output .= '<div id="mywebform-error" style="display: none;"></div>';
      $output .= '<div class="progressbar"><div class="current"><span>0 %</span></div><span class="comment">' . t('Loading...') . '</span></div>';
      $output .= '<form ' . drupal_attributes($form_attr) . '>';
      $output .= '<input type="hidden" name="form_id" value="mywebform-edit-form" />';
      $output .= '<input type="hidden" id="has_errors" name="has_errors" value="" />';
      $output .= '<input type="hidden" id="hash" name="hash" value="' . $this->hash . '" />';
      $output .= '<input type="hidden" id="document_state" name="document_state" value="save" />';
      $output .= '<div id="mywebform-content">';
      $output .= $this->BuildFormHtml($preview);

      $output .= $this->extension;
      $output .= '</div>';
      $output .= '<input id="edit-save-draft" class="form-submit" type="submit" value="' . t('Save as draft') . '" name="op" />';
      $output .= '<input id="edit-submit" class="form-submit" type="submit" value="' . t('Save') . '" name="op" />';
      $output .= '</form>';
      $output .= $this->RenderAttachments();
    }

    foreach($this->webform->fields as $name => $field) {
      if($field->widget == 'container') {
        $this->RenderField($name);
      }
    }

    return $output;
  }

  protected function BuildFormHtml($preview = FALSE) {
    _mywebform_render_fields(NULL, $this, $preview);

    if(!empty($_GET['print'])) {
      return _mywebform_render_document_for_print($this->webform, $this->document);
    }
    else {
      $template = $this->webform->html_template;
    }

    $content = preg_replace_callback("|@([^;]+);|i", '_mywebform_render_fields', $template);
    $content = preg_replace_callback("|\[\"([^\"]*)\"\]|i", '_mywebform_translate', $content);
    return $content;
  }

  public function RenderField($field_name, $index = array(), $preview = FALSE) {
    static $containers = array();
    $output = '';
    $attr = array();
    if(empty($this->webform->fields[$field_name])) {
      return '';
    }
    $field = $this->webform->fields[$field_name];
    if(is_array($field->value)) $field->value = '';

    require_once 'mywebform.fields.inc';
    switch($field->widget) {
    case 'file':
      return _mywebform_render_field_file($field, '', $index, $preview);
    }

    $attr['name'] = $field->name;
    $attr['id'] = $field->name;
    $attr['title'] = $field->title;
    if(!empty($field->help_url)) {
      $url_name = !empty($field->help_url_text) ? $field->help_url_text : $field->help_url;
      if($field->help_url_type == HELP_URL_PATH_RELATIVE) {
        $attr['help_url'] = l($url_name, $field->help_url, array('attributes' => array('target' => 'blank')));
      }
      else {
        $attr['help_url'] = l($url_name, $field->help_url, array('attributes' => array('target' => 'blank'), 'absolute' => TRUE));
      }
    }
    if(!empty($index)) {
      $attr['name'] .= '[' . implode('][', $index) . ']';
      $attr['id'] .= '-' . implode('-', $index);
    } elseif(!empty($field->grid_name)) {
      $attr['name'] .= '[0]';
    }
    $attr['class'] = array();
    if(!empty($field->type)) $attr['class'][] = $field->type;
    if(!empty($field->class)) $attr['class'][] = $field->class;
    $this->SuplyAttributes($attr, $field->attributes);

    $required_span = '<span class="form-required">*</span>';
    $sp_req = '';
    if(!empty($field->required) || !empty($field->may_be_required)) {
      $sp_req = $required_span;
    }

    if($preview) {
      $attr['class'][] = 'form-item';
      $attr['class'][] = implode(' ', $attr['class']);
      switch($field->widget) {
        case 'hidden':
          break;
        case 'select':
          $options = mywebform_build_options($field->options);

          if($field->class == "document_type") {
            $opt = '';
            foreach($options as $key => $value) {
              if(is_array($value)) {
                $opt .= '<optgroup label="' . $key . '">';
                foreach($value as $kk => $vv) {
                  if($kk == $field->value) {
                    $opt .= '<option value="' . $kk . '" selected="selected">' . $vv . '</option>';
                  } else {
                    $opt .= '<option value="' . $kk . '">' . $vv . '</option>';
                  }
                }
                $opt .= '</optgroup>';
              }
              else {
                if($key == $field->value) {
                  $opt .= '<option value="' . $key . '" selected="selected">' . $value . '</option>';
                } else {
                  $opt .= '<option value="' . $key . '">' . $value . '</option>';
                }
              }
            }
            //$options = implode('', $options);


            $output .= sprintf("<select %s>%s</select>", drupal_attributes($attr), $opt);
          }
          else {
            $value = !empty($options[$field->value]) ? $options[$field->value] : $field->value;
            $output .= sprintf('<span %s>%s</span>', drupal_attributes($attr), $value);
          }
          break;
        case 'checkbox':
          $value = '';
          if(!empty($attr['radio'])) {
            if(isset($attr['value'])) {
              if($field->value == $attr['value']) {
                $value = '√';
              }
            }
            else {
              $value = '√';
            }
          }
          elseif($field->value) {
            $value = '√';
          }

          $output .= sprintf('<span %s>%s</span>', drupal_attributes($attr), $value);
          break;
        case 'container':
          $container = mywebform_build_container($field->name, $field->options);
          drupal_add_js($container, 'inline');
          break;
        default:
          $output .= sprintf('<span %s>%s</span>', drupal_attributes($attr), $field->value);
      }
    } else {
      $attr['disabled'] = $field->disabled ? 'disabled' : '';
      $attr['readonly'] = $field->readonly ? 'readonly' : '';
      $attr['minlength'] = $field->minlength;
      $attr['maxlength'] = $field->maxlength;
      $attr['maxlength_semicolon'] = isset($field->maxlength_semicolon) ? $field->maxlength_semicolon : '';
      if(!empty($field->autofield)) $attr['autofield_func'] = isset($field->autofield) ? $field->autofield : '';
      $attr['check_type'] = $field->type;
      if($field->type == 'custom' && !empty($field->format)) $attr['format'] = $field->format;
      if($field->required) {
        $attr['class'][] = 'required';
      }
      if(!empty($field->error)) $attr['class'][] = 'error';
      $attr['class'] = array_unique($attr['class']);
      $attr = array_filter($attr);

      switch($field->widget) {
        case 'hidden':
          $attr['value'] = htmlspecialchars_decode($field->value);
          $output .= sprintf("<input type=\"hidden\" %s />", drupal_attributes($attr));
          break;
        case 'textfield':
          $attr['value'] = htmlspecialchars_decode($field->value);
          $output .= sprintf("<input type=\"text\" %s />%s", drupal_attributes($attr), $sp_req);
          break;
        case 'textarea':
          $output .= sprintf("<textarea %s>%s</textarea>%s", drupal_attributes($attr), htmlspecialchars_decode($field->value), $sp_req);
          break;
        case 'checkbox':
          if(!empty($attr['radio'])) {
            if(isset($attr['value'])) {
              if($field->value == $attr['value']) {
                $attr['checked'] = 'checked';
              }
            }
            else {
              $attr['checked'] = 'checked';
            }
          }
          elseif($field->value) {
            $attr['checked'] = 'checked';
          }
          $output .= sprintf("<input type=\"checkbox\" %s />%s", drupal_attributes($attr), $sp_req);
          break;
        case 'select':
          // Containers
          if(!empty($field->grid_name) && !empty($field->container)) {
            $attr['container'] = $field->container;

            if(empty($containers[$field->container])) {
              $options = mywebform_build_options($field->options);
              $containers[$field->container] = $options;
              $container = "var " . $field->container . " = {\n";
              $i = 0;
              foreach($options as $key => $value) {
                $container .= "	'row_" . $i++ . "' : {'key' : '" . $key . "', 'value' : '" . $value . "'},\n";
              }
              $container .= "};\n";
              drupal_add_js($container, 'inline');
            }

            if($field->value) {
              $options = array($field->value, $options[$field->value]);
            } else {
              $options = array('' => '--------------');
            }
          } else {
            $options = mywebform_build_options($field->options);
          }
          $opt = '';
          foreach($options as $key => $value) {
            if(is_array($value)) {
              $opt .= '<optgroup label="' . $key . '">';
              foreach($value as $kk => $vv) {
                if($kk == $field->value) {
                  $opt .= '<option value="' . $kk . '" selected="selected">' . $vv . '</option>';
                } else {
                  $opt .= '<option value="' . $kk . '">' . $vv . '</option>';
                }
              }
              $opt .= '</optgroup>';
            }
            else {
              if($key == $field->value) {
                $opt .= '<option value="' . $key . '" selected="selected">' . $value . '</option>';
              } else {
                $opt .= '<option value="' . $key . '">' . $value . '</option>';
              }
            }
          }
          //$options = implode('', $options);
          $output .= sprintf("<select %s>%s</select>%s", drupal_attributes($attr), $opt, $sp_req);
          break;
        case 'container':
          $container = mywebform_build_container($field->name, $field->options);
          drupal_add_js($container, 'inline');
          break;
        default:
          $output .= $field->value;
      }
    }

    return $output;
  }

  protected function RenderAttachments() {
    $output = '';
    $files = array();

    if(!empty($this->webform->files['doc'])) {
      $output .= '<div class="attachments">';
      $output .= '<p><b>' . t('Attached documents') . ':</b></p>';
      $output .= '<table border="0" cellpadding="0" cellspacing="5">';
      $output .= '<tbody>';
      $attr = array(
        'html' => TRUE,
        'language' => language_default(),
        'attributes' => array(
          'class' => array('file'),
          '_target' => 'blank',
        ),
      );

      foreach($this->webform->files['doc'] as $file) {
        $output .= '<tr>';
        $output .= '<td valign="top">';
        $temp_file = file_load($file->fid);
        //$path = mywebform_files_dir() . '/' . $file->filename;

          $title = explode('.', $temp_file->filename);
          array_pop($title);
          $title = str_replace('_', ' ', implode('.', $title));
          $attr['attributes']['class'][] = 'file';
          switch($temp_file->filemime) {
            case 'application/pdf': $attr['attributes']['class'][] .= ' pdf'; break;
            case 'application/msword': $attr['attributes']['class'][] .= ' doc'; break;
          }
          $output .= l('&nbsp;', file_create_url($temp_file->uri), $attr);
          $output .= '</td>';
          $output .= '<td valign="top" width="100%">';
          $attr['attributes']['class'][] = 'name';
          $output .= l($title, file_create_url($temp_file->uri), $attr);
          $output .= '<br>' . t('Created') . ': <span style="color: rgb(110, 110, 110);">' . format_date($temp_file->timestamp, 'custom', "d/m/Y") . '</span>';
          $output .= '</td>';
          $output .= '</tr>';
        }

      $output .= '</tbody>';
      $output .= '</table>';
      $output .= '</div>';
    }
    return $output;
  }

  public static function SuplyAttributes(&$attr, $field_attributes) {

    $field_attributes = array_filter(array_map('trim', explode("\n", $field_attributes)));
    foreach($field_attributes as $data) {
      list($key, $value) = array_map('trim', explode("|", $data, 2));

      if(isset($attr[$key])) {
        if(is_array($attr[$key])) {
          $attr[$key][] = $value;
        }
        else {
          $attr[$key] = $value;
        }
      }
      else {
        $attr[$key] = $value;
      }
    }
  }

  protected function BuildAutofields() {
    $output = '';

    $formulas = $this->GetAutofieldsFormulas();

    foreach($formulas as $formula) {
      foreach($formula['fields'] as $field) {
        if(substr($field, 0, 1) == '&') {
          $field = substr($field, 1);
        }

        if(isset($this->webform->fields[$field])) {
          if(empty($this->webform->fields[$field]->autofield)) {
            $this->webform->fields[$field]->autofield = $formula['func_name'];
          } else {
            $this->webform->fields[$field]->autofield .= ' ' . $formula['func_name'];
          }
        }
      }
    }

    if(!empty($formulas)) {
      $output .= "Drupal.behaviors.autofields = function() {\n";
      foreach($formulas as $formula) {
        $output .= $formula['call'] . "\n";
      }
      $output .= "}\n\n";

      foreach($formulas as $formula) {
        $output .= $formula['function'] . "\n";
      }

      foreach($formulas as $formula) {
        $output .= $formula['check_function'] . "\n";
      }

      $output .= "
webformValidator.validators.validateAutofields = function(v, allowOverpass) {
  var messages = '';
  var result = 0;
  var curres = 0;
  var N=0;
      ";
      foreach($formulas as $formula) {
        $output .= $formula['validate'];
      }
      $output .= "
    if(messages != '') {
      if(!allowOverpass) return false;
      if(!confirm('" . t('There are some autocalculation exceptions') . ":\\n\\n' + messages + '\\n". t('Do you wish to continue?') . "')) {
        return false;
      }
    }

    return true;
  }";

      drupal_add_js($output, 'inline');
    } else {
      // Dummy function to prevent JS error "function not found"
      $output .= "function validate_autofields(show_confirm) {return true;}\n";
    }
  }

  protected function GetAutofieldsFormulas() {
    $formulas = array();
    $autofields = $this->ClearAutofieldsComments($this->webform->autofields);
    $autofields = array_filter(array_map('trim', explode(";", $autofields)));

    foreach($autofields as $item) {
      $formula = array();

      if(preg_match_all("|(.*)=[ ]*\[([^]]*)\]msg\((.*)\)|i", $item, $matches)) {
        $formula['formula'] = $matches[1][0];
        $formula['result'] = $matches[2][0];
        $formula['message'] = $matches[3][0];
        if(preg_match_all("|\[([^]]*)\]|i", $formula['formula'], $matches)) {
          $formula['fields'] = array_values(array_unique($matches[1]));
        } else {
          $formula['fields'] = array();
        }
      }
      if(!empty($formula)) {
        $this->AutofieldsJsFunction($formula);
        $formulas[] = $formula;
      }
    }
    return $formulas;
  }

  protected function ClearAutofieldsComments($autofields) {
    $rows = array_map('trim', explode("\n", $autofields));
    foreach($rows as $key => $row) {
      $i = strpos($row, '//');
      if($i !== FALSE) {
        $rows[$key] = trim(substr($row, 0, $i));
      }
    }

    return implode("\n", array_filter($rows));
  }

  protected function AutofieldsJsFunction(&$formula) {
    $result = $formula['result'];
    $by_row_index = FALSE;
    if(preg_match('|(\{n\})$|i', $result)) {
      $result = str_replace(array('{n}', '{N}') , '', $result);
      $by_row_index = TRUE;
    }
    $func_name = str_replace('-', '_', 'update_' . $result);
    $call = array();

    if(!empty($this->webform->fields[$result])) {
      $this->webform->fields[$result]->class .= ' autofield';
    }

    $validate = '';
    $function  = "function " . $func_name . "() {\n";
    if($by_row_index) {
      $function .= "	var N = $(this).attr('row-index');\n";
      $validate .= "	var N = $(this).attr('row-index');\n";
    }

    $n = $by_row_index ? "-'+N" : "'";
    $arg_tr = array();
    foreach($formula['fields'] as $key => $field) {
      if(substr($field, 0, 1) == '&') {
        $field = substr($field, 1);
        $arg_tr['[&' . $field . ']'] = sprintf("$('#%s%s)", $field, $n);
      } else {
        $arg_tr['[' . $field . ']'] = sprintf("toFloat($('#%s%s))", $field, $n);
      }
      if(empty($this->webform->fields[$field]->grid_name)) {
        $call[$field] = "	$('#" .$field . "').bind('change', " . $func_name . ");\n";
      }
    }

    $str = strtr($formula['formula'], $arg_tr);
    $function .= "	var result = " . $str . ";\n";
    $function .= sprintf("	$('#%s%s).val(formatNumber(result, 2)).trigger('change');\n", $result, $n);
    $function .= "}\n";

    $validate .= "	result = " .$str . ";\n";
    $validate .= sprintf("	curres = toFloat($('#%s%s));\n", $result, $n);
    $validate .= "	if(formatNumber(curres, 2) != formatNumber(result, 2)) {\n";
    $msg = "'" . t($formula['message']) . " (' + formatNumber(curres, 2) + ' -> ' + formatNumber(result, 2) + ')'";
    $validate .= "		messages += " . $msg . " + '\\n';\n";
    $validate .= "		v.errors.push(" . $msg . ");\n";
    $validate .= sprintf("		$('#%s%s).addClass('error');\n", $result, $n);
    $validate .= "	}\n";

    $formula['validate'] = $validate;
    $formula['function'] = $function;
    $formula['func_name'] = $func_name;
    $formula['call'] = implode('', $call);
  }
}

class WebFormException extends Exception {
}

function mywebform_sanitize_xml($xml, $tech_name) {
  try {
    $form = new WebForm($tech_name, $xml);
    return $form->OutputXML();
  } catch(Exception $e) {}

  return $xml;
}

function _mywebform_render_fields($matches, $form = NULL, $preview = FALSE) {
  static $set_form = NULL;
  static $set_preview = FALSE;

  if(isset($form)) {
    $set_form = $form;
    $set_preview = $preview;
    return;
  }

  $index = explode('[', str_replace(']', '', $matches[1]));
  $field_name = array_shift($index);

  return $set_form->RenderField($field_name, $index, $set_preview);
}

function _mywebform_translate($matches) {
  if(!empty($matches[1])) {
    return mywebform_t($matches[1]);
  }
  return '';
}

/** NOT USED!!!
 * Saves an uploaded file from PHP temp directory to specified one and return a string
 * with information about it
 *
 * @param array $file - an array being a piece of $_FILES for the specific one file
 * @return string - a string with information about the uploaded file in format CHECKSUM;FID;URL
 */
function _mywebform_attach_file($file, $allowed_mime = array(), $max_size = 0) {
  global $user;
  $files = array();
  $result = array();

  // Normalize value
  if(is_array($file['name'])) {
    $i = 1;
    foreach($file['name'] as $key => $value) {
      $files[$i++] = array(
        'name' => $file['name'][$key],
        'type' => $file['type'][$key],
        'tmp_name' => $file['tmp_name'][$key],
        'error' => $file['error'][$key],
        'size' => $file['size'][$key],
      );
    }
  } else {
    $files[1] = $file;
  }

  // Upload files and fill results
  foreach($files as $key => $f) {
    //if(empty($f['name'])) { $result[$key] = ''; continue; }
    //if($f['error'] != 0) { $result[$key] = ''; continue; }
    //if(!empty($allowed_mime) && !in_array($f['type'], $allowed_mime))  { $result[$key] = ''; continue; }
    //if($max_size && $f['size'] >= $max_size)  { $result[$key] = ''; continue; }

    $obj = new stdClass();
    $obj->uid = $user->uid;
    $obj->filename = $f['name'];
    $obj->filemime = file_get_mimetype($obj->filename);
    $obj->filesize = $f['size'];

    $filepath = variable_get('mywebform_document_attachment_folder','private://attachments');

    file_prepare_directory($filepath, FILE_CREATE_DIRECTORY);
    file_prepare_directory($filepath, FILE_MODIFY_PERMISSIONS);

    $obj->uri = $filepath . '/' . $obj->filename;
    $obj->uri = file_create_filename($obj->filename, $obj->uri);

    if (!variable_get('allow_insecure_uploads', 0) && preg_match('/\.(php|pl|py|cgi|asp|js)(\.|$)/i', $obj->filename) && (substr($obj->filename, -4) != '.txt')) {
      $obj->filemime = 'text/plain';
      $obj->uri .= '.txt';
      $obj->filename .= '.txt';
      // The .txt extension may not be in the allowed list of extensions. We have
      // to add it here or else the file upload will fail.
    }

    if (!drupal_move_uploaded_file($f['tmp_name'], $obj->uri)) {
      $result[$key] = ''; continue;
    }

    $obj->status = FILE_STATUS_PERMANENT;

    file_save($obj);

    $md5 = md5_file(file_create_url($obj->uri));
    $url = url('getfile', array(array('absolute' => TRUE, 'language' => language_default, 'query' => array('fid' => $obj->fid))));
    $result[$key] = $md5 . ';' . $obj->fid . ';' . $url;
  }

  return is_array($file['name']) ? $result : $result[1];
}

/**
 * Delete an attached file
 *
 * @param int $fid - File's identifier
 */
function _mywebform_delete_file($fid) {
  if($fid) {
    $file = file_load($fid);
    if($file !== FALSE) {
      $file->status = 0;
      file_save($file);
    }
  }
}

/**
 * Generates a $_POST-like array from a xml string, based on fields data
 *
 * @param string $xml - a string containing xml data to parse
 * @return an associative array of VARNAME<->VALUE
 */
function _mywebform_xml_to_post($xml, $fields) {
  $post = array();
  $pre_post = array();
  $field_keys = array();

  // Parse string with SimpleXML to an array of xpaths without first node
  $xml = simplexml_load_string(mywebform_clear_xml($xml));
  if($xml === FALSE) return $post;
  $xml_name = $xml->getName();
  foreach($xml->children() as $node) {
    $pre_post = array_merge($pre_post, _mywebform_xml_children($node));
  }

  // Get field xpath <-> name corelation
  foreach($fields as $name => $field) {
    if(!empty($field->xpath)) {
      $field_keys[$field->xpath] = $name;
    }
  }

  // Recover POST
  foreach($pre_post as $key => $value) {
    $name = $xml_name . '/' . $key;
    if(isset($field_keys[$name])) {
      $name = $field_keys[$name];
      if(is_array($value)) {
        $post[$name] = array();
        foreach($value as $key => $val) {
          $post[$name][(int) $key] = $val;
        }
      } else {
        $post[$name] = $value;
      }
    }
  }

  return $post;
}

/**
 * Recursively build an XPATH<->VALUE array from XML tree
 *
 * @param SimpleXML $node - current node to parse
 * @return an associative array of XPATH<->VALUE
 */
function _mywebform_xml_children($node) {
  if(!count($node->children())) {
    $value = (string) $node;
    return array($node->getName() => $value);
  }

  $post = array();
  $attr = NULL;
  $index = NULL;

  foreach($node->attributes() as $name => $value) {
    $attr = $name;
    $index = ' ' . (string) $value;
    break;
  }

  $name = $node->getName();
  if(!empty($attr)) {
    $name .= '[' . $attr . ']';
  }

  foreach($node->children() as $child) {
    if(is_array($child)) {
      foreach($child as $sibling) {
        $post = array_merge_recursive($post, _mywebform_xml_children($sibling));
      }
    } else {
      $post = array_merge_recursive($post, _mywebform_xml_children($child));
    }
  }

  $p = array();
  foreach($post as $key => $value) {
    if($index) {
      $p[$name . '/' . $key][$index] = $value;
    } else {
      $p[$name . '/' . $key] = $value;
    }
  }
  return $p;
}

function _mywebform_complex_set_value(&$complex, $path, $value) {
  if(!count($path)) {
    $complex['#value'] = $value;
  } else {
    $current = array_shift($path);
    if(!isset($complex[$current])) {
      $complex[$current] = array();
    }
    _mywebform_complex_set_value($complex[$current], $path, $value);
  }
}

function _mywebform_complex_to_xml($complex, $step = 0) {
  $xml = '';
  $repeat = str_repeat('  ', $step);

  foreach($complex as $key => $value) {
    if(is_array($value)) {
      $attr = '';
      $inner = '';
      $a = explode(' ', $key);
      $key_close = $a[0];

      if(isset($value['#attr'])) {
        $attr = $value['#attr'];
        unset($value['#attr']);
        if(is_array($attr)) {
          foreach($attr as $n => $v) {
            $attr[$n] = $n . '="' . $v . '"';
          }
          $attr = trim(implode(' ', $attr));
        }
        if(!empty($attr)) $attr = ' ' . $attr;
      }
      if(isset($value['#value'])) {
        $inner = $value['#value'];
        unset($value['#value']);
      }
      if(empty($value)) {
        if(!strlen((string) $inner)) {
          $xml .= sprintf("%s<%s%s />\n", $repeat, $key, $attr);
        } else {
          $xml .= sprintf("%s<%s%s>%s</%s>\n", $repeat, $key, $attr, $inner, $key_close);
        }
      } else {
        $xml .= sprintf("%s<%s%s>%s\n%s%s</%s>\n", $repeat, $key, $attr, $inner, _mywebform_complex_to_xml($value, $step + 1), $repeat, $key_close);
      }
    }
  }

  return $xml;
}

function _mywebform_check_cyr_with_latin($str) {
  if(preg_match("/[а-я]+/ui", $str)) {
    if(preg_match("/[a-z]+/ui", $str)) {
      return TRUE;
    }
  }
  return FALSE;
}

function _mywebform_fix_cyr_with_latin($str) {
  static $msg = FALSE;

  if(preg_match("/[а-я]+/ui", $str)) {
    if(preg_match("/[a-z]+/ui", $str)) {
      $tr = array('А' => 'A', 'В' => 'B', 'Е' => 'E', 'З' => '3', 'К' => 'K', 'М' => 'M', 'Н' => 'H', 'О' => 'O',
        'Р' => 'P', 'С' => 'C', 'Т' => 'T', 'У' => 'Y', 'Х' => 'X', 'Я' => 'R', 'а' => 'a', 'в' => 'b', 'е' => 'e',
        'з' => '3', 'к' => 'k', 'м' => 'm', 'н' => 'h', 'о' => 'o', 'р' => 'p', 'с' => 'c', 'т' => 't', 'у' => 'y',
        'х' => 'x', 'г' => 'r', );
      if(!$msg) {
        drupal_set_message(t('The document contained wrong cyrilic values. The content has been fixed. Check it once more to ensure that it has no more mistakes!!!'));
        $msg = TRUE;
      }
      return strtr($str, $tr);
    }
  }
  return $str;
}

function _mywebform_render_document_for_print($webform, $document) {
  $account = user_load($document->author_uid);
  $path = getcwd() . base_path() . PRINT_TEMPLATE_PATH . '/' . $webform->tech_name . '.tpl.php';
  $country_md_code = 'MD';
  ob_start();
  if(file_exists($path)) {
    include_once($path);
  }
  $content = ob_get_contents();
  ob_clean();
  $content = _mywebform_get_print_header($content);

  return $content;
}

function _get_document_attachments($webform) {
  $count = 0;
  $options = mywebform_build_options($webform->fields['doc_dinamic_doc_type']->options);
  $left = '';
  $right = '';
  $table_left = '';
  $table_right = '';
  $output = array();
  if(is_array($options)) {
    foreach($options as $k => $v) {
      if(is_array($v)) {
        foreach($v as $kk => $vv){
          $count++;
        }
      }
      else {
        $count++;
      }
    }
  }
  if(is_array($webform->fields['doc_dinamic_doc_type']->value)) {
    foreach($webform->fields['doc_dinamic_doc_type']->value as $k => $v) {
      if($v == '1') {
        $count++;
      }
    }
  }

  $count = $count - 1;

  if($count > 0) {
    $middle = ceil($count / 2);
  }

  if(is_array($options)) {
    $i = 1;
    foreach($options as $k => $v) {
      if(empty($v)) continue;
      $table_data = '';
      $pages = '';
      $inline_count = 0;
      $index = 0;
      if(is_array($v)) {
        $other_title = '<tr><td><strong>' . $k . '</strong></td><td>&nbsp;</td></tr>';
        foreach($v as $kk => $vv) {
          $table_data = '';
          if(is_array($webform->fields['doc_dinamic_doc_type']->value)) {
            if(in_array($kk, $webform->fields['doc_dinamic_doc_type']->value)) {
              if($kk == '1')	continue;
              $index = array_search($kk, $webform->fields['doc_dinamic_doc_type']->value);
              //if(empty($_GET['pdf'])) {

              $temp_val = @$webform->fields['doc_dinamic_doc_document']->value[$index];
              @list($md5, $fid, $url) = @explode(';', $temp_val, 3);
              if(!empty($fid)) {
                $doc_link = l($vv, 'getfile', array('query' => array('fid' => $fid), 'absolute' => TRUE));
              }
              else {
                $doc_link = $vv;
              }

              $table_data .= '<tr><td>' . '<input type="checkbox" disabled="disabled" checked="checked"/> ' . $doc_link . '</td>';

              if($index) {
                $pages = @$webform->fields['doc_dinamic_doc_pages']->value[$index];
              }
              $table_data .= '<td>' . $pages . '</td></tr>';
            }
            else {
              $table_data .=  '<tr><td>' . '<input type="checkbox" disabled="disabled"/> ' . $vv . '</td><td>&nbsp;</td></tr>';
            }
            if(!empty($other_title)) {
              $table_data = $other_title . $table_data;
              $other_title = '';
            }
            $inline_count++;
            if($i >= $middle) {
              $right .= $table_data;
            }
            else {
              $left .= $table_data;
            }
            $i++;
          }
        }
      }
      else {
        if(is_array($webform->fields['doc_dinamic_doc_type']->value)) {
          if(in_array($k, $webform->fields['doc_dinamic_doc_type']->value)) {
            if($k == '1') continue;
            $index = array_search($k, $webform->fields['doc_dinamic_doc_type']->value);

            $temp_val = @$webform->fields['doc_dinamic_doc_document']->value[$index];
            @list($md5, $fid, $url) = @explode(';', $temp_val, 3);
            if(!empty($fid)) {
              $doc_link = l($v, 'getfile', array('query' => array('fid' => $fid), 'absolute' => TRUE));
            }
            else {
              $doc_link = $v;
            }
            $table_data .= '<tr><td>' . '<input type="checkbox" disabled="disabled" checked="checked"/> ' . $doc_link . '</td>';

            if($index) {
              $pages = @$webform->fields['doc_dinamic_doc_pages']->value[$index];
            }
            $table_data .= '<td class="center">' . $pages . '</td></tr>';
          }
          else {
            $table_data .=  '<tr><td>' . '<input type="checkbox" disabled="disabled"/> ' . $v . '</td><td>&nbsp;</td></tr>';
          }

          if($i >= $middle) {
            $right .= $table_data;
          }
          else {
            $left .= $table_data;
          }
          $i++;
        }
      }

    }
    if(is_array($webform->fields['doc_dinamic_doc_type']->value)) {
      $files_other_type = '';
      foreach($webform->fields['doc_dinamic_doc_type']->value as $k => $v) {
        if($v == '1') {
          $temp_val = @$webform->fields['doc_dinamic_doc_document']->value[$k];
          @list($md5, $fid, $url) = @explode(';', $temp_val, 3);
          if(!empty($fid)) {
            $doc_link = l(@$webform->fields['doc_dinamic_doc_another_type']->value[$k], 'getfile', array('query' => array('fid' => $fid), 'absolute' => TRUE));
          }
          else {
            $doc_link = @$webform->fields['doc_dinamic_doc_another_type']->value[$k];
          }

          $files_other_type .= '<tr><td>' . '<input type="checkbox" disabled="disabled" checked="checked"/> ' . $doc_link . '</td>';
          $files_other_type .= '<td class="center">' . @$webform->fields['doc_dinamic_doc_pages']->value[$k] . '</td></tr>';
        }
      }
      $right .= $files_other_type;
    }
  }

  $output['table_left'] = $left;
  $output['table_right'] = $right;

  return $output;
}

function _mywebform_get_print_header($content) {
  global $language;
  $output = '';
  if(!empty($_GET['pdf'])) {
    $output .= '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML+RDFa 1.0//EN"
      "http://www.w3.org/MarkUp/DTD/xhtml-rdfa-1.dtd">
      <html xmlns="http://www.w3.org/1999/xhtml" xml:lang="' . $language->language . '" version="XHTML+RDFa 1.0">';

    $output .= '<head>';
    $output .= '<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />';
    $output .= drupal_get_css();
    $output .= drupal_get_js();
    $output .= '</head>';
    $output .= '
    <body>
      <div id="page-wrapper">
        <div id="page">
          <div id="main-wrapper" class="clearfix">
            <div id="content" class="column" style="width:100%">
              <div class="section">
                <div id="mywebform-content">' . $content . '</div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </body>';
    $output .= '</html>';
  }
  else {
    $output .= $content;
  }

  return $output;
}
