<?php

class WebForm {
  public $data = array();
  public $resultset = array();
  public $input = array();
  // Webform's object
  public $webform;
  public $extension = '';
  // Document's XML data
  protected $xml;
  // Form's submit callback
  protected $submit_callback;
  // Form's validate callback
  protected $validate_callback;
  // Module's predefined values to substitute the form items' values
  protected $predefined_values;
  protected $cancel_url;
  // Holds warnings that may be downloaded or uploaded from other module. Format [{fieldName: '%s', index: %d, msg: '%s'}, ...]
  protected $warnings = array();
  // Whenever to revalidate entire form or just the modified fields
  protected $revalidate = FALSE;

  private $author_uid = NULL;

  public static function SuplyAttributes(&$attr, $field_attributes) {
    $field_attributes = array_filter(array_map('trim', explode("\n", $field_attributes)));
    foreach ($field_attributes as $data) {
      list($key, $value) = array_map('trim', explode(";;", $data, 2));
      if (empty($attr[$key])) {
        $attr[$key] = $value;
      }
      else {
        $attr[$key] .= ' ' . $value;
      }
    }
  }

  /**
   * Class constructor - initializes the object and gets the POST values
   * @param string $webform_tech_name
   *   The webform's technical name
   * @param string $xml
   *   The XML with data
   * @param string $submit_callback
   *   The function called to submit the form
   * @param string $author_uid
   *   The user id of the author
   */
  public function __construct($webform_tech_name, $author_uid = NULL, $submit_callback = NULL, $validate_callback = NULL) {
    $this->webform = mywebform_load($webform_tech_name);
    $this->author_uid = $author_uid;
    if (empty($this->webform)) {
      throw new WebFormException('Webform not found');
    }
    $this->submit_callback = $submit_callback;
    $this->validate_callback = $validate_callback;
  }

  public function setXML($xml, $override = TRUE) {
    $this->fillXmlValues($xml, $override);
    $this->fillPostValues();
    $this->sanitizeValues();
  }

  public function setDataFromArray($data, $override = TRUE) {
    $this->fillArrayValues($data, $override);
    $this->sanitizeValues();
  }

  public function validateForm() {
    $this->validateFields();
    $this->validateAutofields();
  }

  public function setWarnings($warnings, $revalidate = FALSE) {
    $this->warnings = $warnings;
    $this->revalidate = $revalidate;
  }

  public function setCancelPath($cancel_url) {
    $this->cancel_url = $cancel_url;
  }

  public function Render($preview = FALSE) {
    $empty_fields = array();
    foreach ($this->webform->fields as $name => $field) {
      if (!empty($field->default_value)) {
        if (!isset($field->value) || empty($field->value)) {
          $empty_fields[] = $field->default_value;
        }
      }
    }
    $this->predefined_values = mywebform_predefined_values($this->author_uid, $this->webform->tech_name, $empty_fields);
    $this->fillDefaultValues(false);

    if (!$preview) {
      $this->Submit();
    }

    drupal_add_css(drupal_get_path('module', 'mywebform') . '/mywebform.css');
    drupal_add_js(drupal_get_path('module', 'mywebform') . '/mywebform.js');
    drupal_add_js(drupal_get_path('module', 'mywebform') . '/php.js');

    $grids = $this->GetGridList();
    if (!empty($grids)) {
      if ($preview) {
        drupal_add_js(drupal_get_path('module', 'mywebform') . '/grid-view.js');
      }
      else {
        drupal_add_js(drupal_get_path('module', 'mywebform') . '/grid-edit.js');
      }
    }

    if (!$preview) {
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

    if (is_array($this->webform->files['css'])) {
      foreach ($this->webform->files['css'] as $file) {
        if (empty($file->locked)) {
          drupal_add_css(mywebform_files_dir() . '/' . $file->filename);
        }
        else {
          drupal_add_css($file->filename);
        }
      }
    }
    if (is_array($this->webform->files['js'])) {
      foreach ($this->webform->files['js'] as $file) {
        if (empty($file->locked)) {
          drupal_add_js(mywebform_files_dir() . '/' . $file->filename);
        }
        else {
          drupal_add_js($file->filename);
        }
      }
    }

    // Add autofield JS functions
    if (!$preview) {
      $this->BuildAutofields();
    }

    // Set JavaScript variables
    $attr = array(
      'grids' => $grids,
      'print' => ($preview && !empty($_GET['print'])),
      'preview' => (bool) $preview,
      'userprofile' => url('userprofile'),
      'grid_timer' => 100,
      'fields' => array(),
      'values' => array(),
      'errors' => $this->getErrors(),
      'revalidate' => $this->revalidate,
      'may_upload_grid' => (int) (!$preview && $this->webform->may_upload_grid),
      'xml_upload_button' => t('Upload XML'),
      'period' => array(
        'start' => array(
          'day' => (int) ($this->webform->period_start ? date('j', $this->webform->period_start) : date('j')),
          'month' => (int) ($this->webform->period_start ? date('n', $this->webform->period_start) : date('n')),
          'year' => (int) ($this->webform->period_start ? date('Y', $this->webform->period_start) : date('Y') - 20),
          'ts' => (int) ($this->webform->period_start ? $this->webform->period_start : mktime(0, 0, 0, 1, 1, date('Y') - 20)),
        ),
        'end' => array(
          'day' => (int) ($this->webform->period_end ? date('j', $this->webform->period_end) : date('j')),
          'month' => (int) ($this->webform->period_end ? date('n', $this->webform->period_end) : date('n')),
          'year' => (int) ($this->webform->period_end ? date('Y', $this->webform->period_end) : date('Y')),
          'ts' => (int) ($this->webform->period_end ? $this->webform->period_end : time()),
        ),
      ),
      'messages' => array(
        'confirm_period' => t("You are saving the declaration with the fiscal period @period.\nDo you really want to save the declaration?"),
      ),
    );

    foreach ($this->webform->fields as $name => $field) {
      if ($preview && !empty($field->grid_name) && $field->widget == 'select' && !empty($field->value)) {
        if ($field->format != 'key') {
          $options = mywebform_build_options($field->options);
          foreach ($field->value as $key => $val) {
            $field->value[$key] = isset($options[$val]) ? $options[$val] : $val;
          }
        }
      }

      $attr['fields'][$name] = array(
        'name' => $name,
        'title' => $field->title,
        'grid_name' => $field->grid_name,
        'type' => $field->type,
        'widget' => $field->widget,
        'format' => $field->format,
        'disabled' => (int) $field->disabled,
        'readonly' => (int) $field->readonly,
        'required' => (int) $field->required,
        'class' => $field->class,
        'minlength' => (int) $field->minlength,
        'maxlength' => (int) $field->maxlength,
        'decimal_length' => (int) $field->decimal_length,
        'error' => $field->error,
        'autofield' => $field->autofield,
      );

      if (empty($field->grid_name)) {
        if (empty($field->value)) {
          if ($field->value === '0') {
            $attr['values'][$name] = 0;
          }
          else {
            $attr['values'][$name] = '';
          }
        }
        else {
          $attr['values'][$name] = $field->value;
        }
        if($field->type == 'money' && $field->aux['text_format_in_preview'] == '1' && !empty($field->value)) {
          $attr['values'][$name] = numberToString($field->value);
        }
      }
      else {
        $attr['values'][$name] = empty($field->value) ? array() : $field->value;
      }
      if (is_array($attr['values'][$name])) {
        $attr['values'][$name] = array_values($attr['values'][$name]);
        if ($field->widget == 'file') {
          $attr['values'][$name . '_files'] = array();
          foreach ($attr['values'][$name] as $key => $value) {
            list($md5, $fid, $url) = explode(';', $value, 3);
            $file = db_fetch_object(db_query("SELECT fid, filename, filepath, filesize FROM {files} WHERE fid = %d", $fid));
            if (!empty($file)) {
              $arr = explode('.', $file->filename);
              $file->ext = strtolower(end($arr));
              $file->filelink = l($file->filename, $file->filepath);
              $file->size = format_size($file->filesize);
            }
            $attr['values'][$name . '_files'][$key] = $file;
          }
        }
      }
    }

    drupal_add_js(array('mywebform' => $attr), 'setting');

    $output = '';

    if ($preview) {
      /*if (empty($_GET['print'])) {
        $output .= '<div class="forprint">' . l(t('Printer version'), $_GET['q'], array('query' => array('print' => '1'))) . '</div>';
      }*/
      $output .= '<div class="progressbar"><div class="current"><span>0 %</span></div><span class="comment">' . t('Loading...') . '</span></div>';
      $output .= '<div id="mywebform-content" class="preview">';
      $output .= $this->BuildFormHtml($preview);
      $output .= '</div>';
    }
    else {
      if ($this->webform->may_upload_grid) {
        $output .= '<div id="mywebform-upload-grid-xml-form-wrapper" style="display: none;">';
        $output .= drupal_get_form('mywebform_upload_grid_xml_form', $this);
        $output .= '</div>';
      }

      $error = $this->getErrorsAsText();
      $class = empty($error) ? ' style="display: none;"' : '';
      $error = empty($error) ? '' : theme('item_list', $error);
      $output .= '<div id="mywebform-error"' . $class . '>' . $error . '</div>';

      $output .= '<div class="progressbar"><div class="current"><span>0 %</span></div><span class="comment">' . t('Loading...') . '</span></div>';

      $form_attr = array(
        'id' => 'mywebform-edit-form',
        'method' => 'post',
        'enctype' => 'multipart/form-data',
        'accept-charset' => 'UTF-8',
        'action' => url($_GET['q']) . (!empty($_GET['destination']) ? '?' . drupal_get_destination() : '' ),
      );
      $output .= '<form ' . drupal_attributes($form_attr) . '>';
      $output .= '<input type="hidden" name="form_id" value="mywebform-edit-form" />';
      $output .= '<input type="hidden" id="warnings" name="warnings" value="" />';
      $output .= '<input type="hidden" id="js_ok" name="js_ok" value="0" />';
      $output .= '<input type="hidden" id="serialized_data" name="serialized_data" value="" />';
      $output .= '<div id="mywebform-content">';
      $output .= $this->BuildFormHtml($preview);
      $output .= $this->extension;
      $output .= '</div>';
      $output .= '<div class="form-actions">';
      $output .= '<input type="hidden" id="clicked-button" name="clicked_button" />';
      $output .= '<input id="edit-submit" class="form-submit" type="submit" value="' . t('Save') . '" name="op" />';
      $output .= '<input id="edit-save-close" class="form-submit" type="submit" value="' . t('Save and close') . '" name="op" />';
      $output .= '<input id="edit-save-sign" class="form-submit" type="submit" value="' . t('Save and sign') . '" name="op" />';
      $output .= '<input id="edit-save-sign-send" class="form-submit" type="submit" value="' . t('Save and sign and send') . '" name="op" />';
      $output .= '<input id="edit-save-send" class="form-submit" type="submit" value="' . t('Save and send') . '" name="op" />';
      if (!empty($this->cancel_url)) {
        $output .= l(t('Close'), $this->cancel_url, array('attributes' => array('class' => 'form-cancel')));
      }
      $output .= '</div>';
      $output .= '</form>';
      $output .= $this->RenderAttachments();
    }

    foreach ($this->webform->fields as $name => $field) {
      if ($field->widget == 'container') {
        $this->RenderField($name);
      }
    }

    return $output;
  }

  public function OutputXml() {
    $post = array();
    foreach ($this->webform->fields as $field) {
      if (empty($field->xpath) ||
          ($field->ignore_empty === "1" &&
            (is_array($field->value) && count($field->value)==0 || strlen($field->value)==0)
            )) {
        continue;
      }

      if (preg_match_all("|\[([^]]*)\]|i", $field->xpath, $matches)) {
        if (is_array($field->value)) {
          $key = 0;
          foreach ($field->value as $value) {
            $key++;
            $tr = array();
            foreach ($matches[1] as $attr) {
              $tr['[' . $attr . ']'] = ' ' . $attr . '="' . $key . '"';
            }
            $post[strtr($field->xpath, $tr)] = $value;
          }
        }
      }
      else {
        if (is_array($field->value)) {
          $post[$field->xpath] = trim(implode(' ', $field->value));
        }
        else {
          $post[$field->xpath] = $field->value;
        }
      }
    }

    $complex = array();
    foreach ($post as $name => $value) {
      _mywebform_complex_set_value($complex, explode('/', $name), htmlspecialchars($value));
    }
    $complex['doc']['#attr'] = array('TypeName' => $this->webform->name, 'TypeTechName' => $this->webform->tech_name);
    return '<?xml version="1.0" encoding="UTF-8" ?>' . "\n" . _mywebform_complex_to_xml($complex);
  }

  public function RenderField($field_name, $index = 0, $preview = FALSE) {
    static $containers = array();
    $output = '';
    $attr = array();
    if (empty($this->webform->fields[$field_name])) {
      return '';
    }
    $field = $this->webform->fields[$field_name];

    require_once 'mywebform.fields.inc';

    switch ($field->widget) {
      case 'file':
        return _mywebform_render_field_file($field, $field->value, $index, $preview);
    }

    $attr['name'] = $field->name;
    $attr['title'] = $field->title;
    $attr['id'] = $field->name;
    $attr['field'] = $field->name;

    if (!empty($index)) {
      $attr['name'] .= '[' . $index . ']';
      $attr['id'] .= '-' . $index;
    }
    elseif (!empty($field->grid_name)) {
      $attr['name'] .= '[0]';
    }
    $attr['class'] = array();
    if ($field->class) {
      $attr['class'][] = $field->class;
    }
    if ($field->type) {
      $attr['class'][] = $field->type;
    }

    $value = '';
    if (is_array($field->value)) {
      $value = (isset($field->value[$index]) && strlen($field->value[$index])) ? $field->value[$index] : $field->default_value;
    }
    else {
      $value = (isset($field->value) && strlen($field->value)) ? $field->value : '';
    }

    if($preview && $field->type == 'money' && !empty($value)) {
      $value = str_replace('.', '-', $value);
    }

    if($preview && $field->type == 'money' && $field->aux['text_format_in_preview'] == '1' && !empty($value)) {
      $value = numberToString($value);
    }

    if($preview && $field->type == 'date' && !empty($field->aux['date_format_in_preview']) && !empty($value)) {
      $value = format_date(strtotime($value), 'custom', $field->aux['date_format_in_preview'], NULL, 'ro');
    }

    $value = filter_xss($value);
    if ($preview) {
      unset($attr['name']);
      $attr['class'][] = 'form-item';
      $attr['class'] = implode(' ', $attr['class']);
      switch ($field->widget) {
        case 'hidden':
          //$output .= sprintf('<span %s style="display: none;">%s</span>', drupal_attributes($attr), $value);
          $attr['value'] = htmlspecialchars_decode($value);
          $output .= sprintf("<input type=\"hidden\" %s />", drupal_attributes($attr));
          break;
        case 'container':
          break;
        case 'select':
          if ($field->format != 'key') {
            $options = mywebform_build_options($field->options);
            $value = isset($options[$value]) ? $options[$value] : $value;
          }
          $output .= sprintf('<span %s>%s</span>', drupal_attributes($attr), $value);
          break;
        case 'checkbox':
          $value = $value ? '√' : '';
          $output .= sprintf('<span %s>%s</span>', drupal_attributes($attr), $value);
          break;
        default:
          $output .= sprintf('<span %s>%s</span>', drupal_attributes($attr), $value);
      }
    }
    else {
      if ((!empty($field->error) && !is_array($field->error)) || (!empty($field->warning) && !is_array($field->warning))) {
        $attr['class'][] = 'error';
      }
      self::SuplyAttributes($attr, $field->attributes);
      $attr['disabled'] = $field->disabled ? 'disabled' : '';
      $attr['readonly'] = $field->readonly ? 'readonly' : '';
      $attr['minlength'] = $field->minlength;
      $attr['maxlength'] = $field->maxlength;
      $attr['decimal_length'] = $field->decimal_length;
      if ($field->required) {
        $attr['class'][] = 'required';
      }

      $attr['class'] = implode(' ', $attr['class']);

      $attr = array_filter($attr);

      if ($field->type == 'period_month') {
        $start = 1;
        $end = 12;
        $field->options = '';
        for ($i = $start; $i <= $end; $i++) {
          $i = sprintf("%02d", $i);
          $field->options .= "$i;;$i\n";
        }
      }

      if ($field->type == 'period_quarter') {
        $start = 1;
        $end = 4;
        $field->options = '';
        for ($i = $start; $i <= $end; $i++) {
          $field->options .= "$i;;$i\n";
        }
      }

      if ($field->type == 'period_semester') {
        $start = 1;
        $end = 2;
        $field->options = '';
        for ($i = $start; $i <= $end; $i++) {
          $field->options .= "$i;;$i\n";
        }
      }

      if ($field->type == 'period_year') {
        $start = $this->webform->period_start ? date('Y', $this->webform->period_start) : date('Y') - 10;
        $end = $this->webform->period_end ? date('Y', $this->webform->period_end) : date('Y');
        $field->options = '';
        for ($i = $start; $i <= $end; $i++) {
          $field->options .= "$i;;$i\n";
        }
      }

      if ($field->type == 'period_fond') {
        $start = 1;
        $end = 3;
        $field->options = '';
        for ($i = $start; $i <= $end; $i++) {
          $field->options .= "$i;;$i\n";
        }
      }

      switch ($field->widget) {
        case 'hidden':
          $attr['value'] = htmlspecialchars_decode($value);
          $output .= sprintf("<input type=\"hidden\" %s />", drupal_attributes($attr));
          break;

        case 'textfield':
          $attr['value'] = htmlspecialchars_decode($value);
          $output .= sprintf("<input type=\"text\" %s />", drupal_attributes($attr));
          break;

        case 'textarea':
          $output .= sprintf("<textarea %s>%s</textarea>", drupal_attributes($attr), htmlspecialchars_decode($value));
          break;

        case 'checkbox':
          if ($value) {
            $attr['checked'] = 'checked';
          }
          $output .= sprintf("<input type=\"checkbox\" %s />", drupal_attributes($attr));
          break;

        case 'select':
          // Containers
          if (!empty($field->grid_name) && !empty($field->container)) {
            $attr['container'] = $field->container;

            if (empty($containers[$field->container])) {
              $options = mywebform_build_options($field->options);
              $containers[$field->container] = $options;
              $container = "var " . $field->container . " = {\n";
              $i = 0;
              foreach ($options as $key => $val) {
                $container .= "	'row_" . $i++ . "' : {'key' : '" . $key . "', 'value' : '" . $val . "'},\n";
              }
              $container .= "};\n";
              drupal_add_js($container, 'inline');
            }

            if ($value) {
              $options = array($value, $options[$value]);
            }
            else {
              $options = array('' => '--------------');
            }
          }
          else {
            $options = mywebform_build_options($field->options);
          }

          foreach ($options as $key => $val) {
            $selected = $key == $value ? ' selected="selected"' : '';
            $options[$key] = '<option value="' . $key . '"' . $selected . '>' . $val . '</option>';
          }
          $options = implode("\n", $options);
          $output .= sprintf("<select %s>\n%s\n</select>", drupal_attributes($attr), $options);
          break;

        case 'container':
          $container = mywebform_build_container($field->name, $field->options);
          drupal_add_js($container, 'inline');
          break;

        default:
          $output .= $value;
      }
    }

    return $output;
  }

  public function getErrors() {
    $error = empty($this->warnings) ? array() : $this->warnings;

    foreach ($this->webform->fields as $field) {
      if (!empty($field->error)) {
        if (is_array($field->error)) {
          foreach ($field->error as $key => $value) {
            $error[] = array('fieldName' => $field->name, 'index' => $key, 'msg' => $value);
          }
        }
        else {
          $error[] = array('fieldName' => $field->name, 'index' => 0, 'msg' => $field->error);
        }
      }
      if (!empty($field->warning)) {
        if (is_array($field->warning)) {
          foreach ($field->warning as $key => $value) {
            $error[] = array('fieldName' => $field->name, 'index' => $key, 'msg' => $value);
          }
        }
        else {
          $error[] = array('fieldName' => $field->name, 'index' => 0, 'msg' => $field->warning);
        }
      }
    }

    return $error;
  }

  public function getErrorsAsText() {
    $error = array();

    if (!empty($this->warnings)) {
      foreach ($this->warnings as $warning) {
        if (!empty($warning['msg'])) {
          $field = $this->webform->fields[$warning['fieldName']];
          $name = empty($field->title) ? $field->name : $field->title;
          if (!empty($field->grid_name)) {
            $error[$name . '-' . $warning['index']] = $name . "[" . ($warning['index'] + 1) . "]  - " . t($warning['msg']);
          }
          else {
            $error[$name] = $name . ' - ' . $warning['msg'];
          }
        }
      }
    }

    foreach ($this->webform->fields as $field) {
      if (!empty($field->error)) {
        $name = empty($field->title) ? $field->name : $field->title;
        if (is_array($field->error)) {
          foreach ($field->error as $key => $value) {
            $error[$name . '-' . $key] = $name . "[" . ($key + 1) . "]  - " . $value;
          }
        }
        else {
          $error[$name] = $name . ' - ' . t($field->error);
        }
      }

      if (!empty($field->warning)) {
        $name = empty($field->title) ? $field->name : $field->title;
        if (is_array($field->warning)) {
          foreach ($field->warning as $key => $value) {
            $error[$name . '-' . $key] = $name . "[" . ($key + 1) . "]  - " . $value;
          }
        }
        else {
          $error[$name] = $name . ' - ' . $field->warning;
        }
      }
    }

    return $error;
  }

  protected function Submit() {
    if (!empty($_POST) && $_POST['form_id'] == 'mywebform-edit-form') {
      if (!$_POST['js_ok']) {
        drupal_set_message(t('Javascript execution failed. Try to resave your declaration again! If this error repeats, contact the administrator!'), 'error');
        return;
      }

      // Validate
      $this->warnings = empty($_POST['warnings']) ? array() : json_decode($_POST['warnings'], TRUE);
      $this->validateFields();
      $this->validateAutofields();
      if(!empty($this->validate_callback)) {
        $func = $this->validate_callback;
        $error = $func($this);
        if($error === FALSE) {
          return;
        }
      }

      foreach ($this->webform->fields as $field) {
        if (!empty($field->error)) {
          return;
        }
      }

      // Remove old files
      foreach ($this->webform->fields as $field) {
        if ($field->widget == 'file') {
          if (is_array($field->value)) {
            foreach ($field->value as $key => $val) {
              if (!empty($val)) {
                list($md5, $fid, $url) = explode(';', $val, 3);
                db_query("UPDATE {files} SET status = %d WHERE fid = %d", FILE_STATUS_PERMANENT, $fid);
              }
            }
          }
          else {
            if (!empty($field->value)) {
              list($md5, $fid, $url) = explode(';', $field->value, 3);
              db_query("UPDATE {files} SET status = %d WHERE fid = %d", FILE_STATUS_PERMANENT, $fid);
            }
          }

          if (is_array($field->old_value)) {
            foreach ($field->old_value as $key => $val) {
              if (!empty($val) && $val != $field->value[$key]) {
                list($md5, $fid, $url) = explode(';', $val, 3);
                _mywebform_delete_file($fid);
              }
            }
          }
          else {
            if (!empty($field->old_value) && $field->old_value != $field->value) {
              list($md5, $fid, $url) = explode(';', $field->old_value, 3);
              _mywebform_delete_file($fid);
            }
          }
        }
      }

      // Export values to resultset
      foreach ($this->webform->fields as $field) {
        if ($field->export_result) {
          $this->resultset[$field->name] = $field->value;
        }
      }

      // Submit
      if (function_exists($this->submit_callback)) {
        $func = $this->submit_callback;
        $func($this);
      }
      else {
        drupal_set_message(t('Webform callback function does not exist'), 'error');
      }

      drupal_goto($_GET['q']);
    }
  }

  protected function fillDefaultValues($replace = TRUE) {
    foreach ($this->webform->fields as $name => $field) {
      if (strlen((string) $field->default_value)) {
        if ($field->type == 'checkbox') {
          $value = !empty($field->default_value);
        }
        else {
          $value = trim(strtr($field->default_value, $this->predefined_values));
        }

        if ($replace || (!isset($field->value) || empty($field->value)))
          $field->value = empty($field->grid_name) ? $value : array($value);
      }
      else {
        if ($replace || (!isset($field->value) || empty($field->value)))
          $field->value = empty($field->grid_name) ? '' : array();
      }
    }
  }

  protected function fillXmlValues($xml, $override) {
    if (empty($xml)) {
      return FALSE;
    }

    $xml = simplexml_load_string($xml);

    foreach ($this->webform->fields as $name => $field) {
      if (!$override && ($field->disabled || $field->readonly || $field->force_default)) {
        continue;
      }

      if (empty($field->xpath)) {
        continue;
      }

      if (empty($field->grid_name)) {
        $xpath = '/' . ltrim(str_replace('[line]', '', $field->xpath), '/');
        $value = $xml->xpath($xpath);
        $field->value = empty($value) ? '' : (string) $value[0];
      }
      else {
        $xpath = explode('[line]', $field->xpath);
        $field_name = trim(ltrim($xpath[1], '/'));
        $xpath = '/' . trim($xpath[0]);
        $field->value = array();

        $elements = $xml->xpath($xpath);
        foreach ($elements as $element) {
          $attr = $element->attributes();
          $line = (string) $attr['line'];
          foreach ($element as $item) {
            if ($item->getName() == $field_name) {
              $field->value[$line] = (string) $item;
              break;
            }
          }
          // Using xpath is slower than foreaching between nodes
          //$val = $element->xpath($field_name);
          //$field->value[$line] = empty($val) ? '' : (string) $val[0];
        }
      }
    }
  }

  protected function fillPostValues() {
    if (empty($_POST) || $_POST['form_id'] != 'mywebform-edit-form') {
      return FALSE;
    }

    // Fill $_POST with unserialized data
    if (!empty($_POST['serialized_data'])) {
      $data = json_decode($_POST['serialized_data'], TRUE);
      if (is_array($data)) {
        foreach ($data as $name => $value) {
          if (is_array($value) && !empty($this->webform->fields[$name]->grid_name)) {
            array_unshift($value, '');
            $_POST[$name] = $value;
          }
        }
      }
    }

    foreach ($this->webform->fields as $name => $field) {
      if ($field->disabled || $field->readonly || $field->force_default) {
        continue;
      }

      if ($field->widget == 'checkbox') {
        $field->value = (int) isset($_POST[$name]);
      }
      elseif ($field->widget == 'file') {
        // Save current file to old_value property to farther remove it if a new
        // one is uploaded
        if (isset($_POST[$name][0])) {
          unset($_POST[$name][0]);
        }
        $field->value = $_POST[$name];
        $field->old_value = $_POST[$name];

        if (isset($_FILES[$field->name . '-file'])) {
          $extensions = variable_get('upload_extensions_default', 'jpg jpeg gif png txt html doc xls pdf ppt pps odt ods odp');
          $value = _mywebform_attach_file($_FILES[$field->name . '-file'], array(), (5*1024*1024), $extensions);
          if (is_array($value)) {
            foreach ($value as $key => $val) {
              if (empty($val)) {
                if (!empty($_POST[$name . '-remove'][$key])) {
                  $field->value[$key] = '';
                }
                else {
                  $field->value[$key] = empty($field->old_value[$key]) ? '' : $field->old_value[$key];
                  unset($field->old_value[$key]);
                }
              }
              else {
                $field->value[$key] = $val;
              }
            }
          }
          else {
            if (empty($value)) {
              if (!empty($_POST[$name . '-remove'])) {
                $field->value = '';
              }
              else {
                $field->value = $field->old_value;
                unset($field->old_value);
              }
            }
            else {
              $field->value = $value;
            }
          }
        }
      }
      else {
        if (empty($field->grid_name)) {
          $field->value = strlen((string) $_POST[$name]) ? $_POST[$name] : '';
        }
        else {
          if (isset($_POST[$name]) && is_array($_POST[$name])) {
            $field->value = $_POST[$name];
          }
          else {
            $field->value = array();
          }

          // Drop index 0 from array - it may be present due a JavaScript error
          // during validation form before submision
          if (isset($field->value[0])) {
            unset($field->value[0]);
          }
        }
      }
    }
  }

  protected function fillArrayValues($data, $override) {
    if (empty($data)) {
      return FALSE;
    }


    if (is_array($data)) {
      foreach ($data as $name => $value) {
        if (is_array($value) && !empty($this->webform->fields[$name]->grid_name)) {
          array_unshift($value, '');
          $data[$name] = $value;
        }
      }
    }


    foreach ($this->webform->fields as $name => $field) {
      if (!$override && ($field->disabled || $field->readonly || $field->force_default)) {
        continue;
      }

      if ($field->widget == 'checkbox') {
        $field->value = (int) isset($data[$name]);
      }
      elseif ($field->widget == 'file') {
        // Save current file to old_value property to farther remove it if a new
        // one is uploaded
        if (isset($data[$name][0])) {
          unset($data[$name][0]);
        }
        $field->value = $data[$name];
        $field->old_value = $data[$name];

        if (isset($_FILES[$field->name . '-file'])) {
          $extensions = variable_get('upload_extensions_default', 'jpg jpeg gif png txt html doc xls pdf ppt pps odt ods odp');
          $value = _mywebform_attach_file($_FILES[$field->name . '-file'], array(), (5*1024*1024), $extensions);
          if (is_array($value)) {
            foreach ($value as $key => $val) {
              if (empty($val)) {
                if (!empty($data[$name . '-remove'][$key])) {
                  $field->value[$key] = '';
                }
                else {
                  $field->value[$key] = empty($field->old_value[$key]) ? '' : $field->old_value[$key];
                  unset($field->old_value[$key]);
                }
              }
              else {
                $field->value[$key] = $val;
              }
            }
          }
          else {
            if (empty($value)) {
              if (!empty($data[$name . '-remove'])) {
                $field->value = '';
              }
              else {
                $field->value = $field->old_value;
                unset($field->old_value);
              }
            }
            else {
              $field->value = $value;
            }
          }
        }
      }
      else {
        if (empty($field->grid_name)) {
          $field->value = strlen((string) $data[$name]) ? $data[$name] : '';
        }
        else {
          if (isset($data[$name]) && is_array($data[$name])) {
            $field->value = $data[$name];
          }
          else {
            $field->value = array();
          }

          // Drop index 0 from array - it may be present due a JavaScript error
          // during validation form before submision
          if (isset($field->value[0])) {
            unset($field->value[0]);
          }
        }
      }
    }
  }

  protected function DEPRECATED_LoadValues() {
    // Load values in priority POST > XML > DEFAULT_VALUE
    foreach ($this->webform->fields as $name => $field) {
      if ($submit && !$field->disabled && !$field->readonly) {
        if ($field->widget == 'checkbox') {
          $field->value = (int) isset($_POST[$name]);
        }
        elseif ($field->widget == 'file') {
          // Save current file to old_value property to farther remove it if a new
          // one is uploaded
          // FIXME: Fix this for standalone and grid field
          $this->webform->fields[$name]->old_value = $post[$name];
          $extensions = variable_get('upload_extensions_default', 'jpg jpeg gif png txt html doc xls pdf ppt pps odt ods odp');
          $value = _mywebform_attach_file($_FILES[$field->name . '-file'], array(), (5*1024*1024), $extensions);
          if (is_array($value)) {
            foreach ($value as $key => $val) {
              if (empty($val) && !empty($post[$name][$key]) && empty($_POST[$name . '-remove'][$key])) {
                $value[$key] = $post[$name][$key];
              }
            }
          }

          if (empty($value)) {
            $value = $this->webform->fields[$name]->old_value;
          }
        }
        else {
          if (is_array($_POST[$name])) {

            $field->value = $_POST[$name];

            // Drop index 0 from array - it may be present due a JavaScript error
            // while validation form before submision
            if (isset($field->value[0])) {
              unset($field->value[0]);
            }
          }
          else {
            $field->value = strlen((string) $_POST[$name]) ? $_POST[$name] : '';
          }
        }
      }
      elseif (!empty($xml)) {
        if (!empty($field->grid_name)) {
          $path = explode('[line]', $field->xpath);
          $field_name = trim(ltrim($path[1], '/'));

          if (!isset($this->grids[$field->grid_name])) {
            $this->grids[$field->grid_name] = array();
            $path = '//' . trim($path[0]);

            $elements = $xml->xpath($path);
            foreach ($elements as $elem) {
              $attr = $elem->attributes();
              $row = (string) $attr['line'];
              foreach ($elem->children() as $item) {
                $this->grids[$field->grid_name][$row][$item->getName()] = (string) $item;
              }
            }
          }

          $field->value = array();
          foreach ($this->grids[$field->grid_name] as $row => $data) {
            $field->value[] = $data[$field_name];
          }
        }
        else {
          if ($field->xpath) {
            $value = $xml->xpath('//' . ltrim(str_replace('[line]', '', $field->xpath), '/'));
          }
          else {
            $value = '';
          }
          if (empty($value)) {
            $field->value = NULL;
          }
          elseif (empty($field->grid_name)) {
            $field->value = (string) $value[0];
          }
          else {
            foreach ($value as $key => $val) {
              $field->value[$key + 1] = (string) $val;
            }
          }
        }
      }
      else {
        if (!empty($field->grid_name)) {
          $field->value = array();
        }
      }

      // Fill field with default value if is NULL
      if (strlen((string) $field->default_value) && !isset($field->value)) {
        if ($field->type == 'checkbox') {
          $field->value = !empty($field->default_value);
        }
        else {
          $field->value = strtr($field->default_value, $this->predefined_values);
        }
      }

      // Force value from default
      if ($field->force_def_val) {
        if ($field->type == 'checkbox') {
          $field->value = !empty($field->default_value);
        }
        else {
          $field->value = strtr($field->default_value, $this->predefined_values);
        }
      }

      $field->value = is_array($field->value) ? array_map('trim', $field->value) : trim($field->value);
    }
  }

  protected function sanitizeValues() {
    foreach ($this->webform->fields as $field) {
      $value = $field->value;

      if (!strlen((string) $value)) {
        $field->value = empty($field->grid_name) ? '' : array();
        continue;
      }

      switch ($field->type) {
        case 'float':
          if (is_array($value)) {
            foreach ($value as $k => $v) {
              if ($v === '') {
                $value[$k] = '';
              }
              else {
                $value[$k] = round(str_replace(',', '.', $v), $field->decimal_length);
              }
            }
          }
          else {
            $value = round(str_replace(',', '.', $value), $field->decimal_length);
          }
          break;

        case 'money':
          if (is_array($value)) {
            foreach ($value as $k => $v) {
              $value[$k] = strlen($v) ? sprintf("%1.2f", str_replace(',', '.', $v)) : '';
            }
          }
          else {
            $value = strlen($value) ? sprintf("%1.2f", str_replace(',', '.', $value)) : '';
          }
          break;

        case 'text':
          if (is_array($value)) {
            foreach ($value as $k => $v) {
              if (_mywebform_check_cyr_with_latin($v)) {
                $value[$k] = _mywebform_fix_cyr_with_latin($v);
              }
            }
          }
          else {
            if (_mywebform_check_cyr_with_latin($value)) {
              $value = _mywebform_fix_cyr_with_latin($value);
            }
          }
          break;

        case 'date':
          if (is_array($value)) {
            foreach ($value as $k => $v) {
              if (!empty($v)) {
                $value[$k] = $this->fixDateValue($v);
              }
            }
          }
          else {
            if (!empty($value)) {
              $value = $this->fixDateValue($value);
            }
          }
          break;

        case 'custom':
          if (!empty($field->format) && $field->format != 'key') {
            if (is_array($value)) {
              foreach ($value as $k => $v) {
                $value[$k] = sprintf($field->format, $v);
              }
            }
            else {
              $value = sprintf($field->format, $value);
            }
          }
          break;
      }

      if ($field->widget == 'select' && !empty($field->options)) {
        $options = mywebform_build_options($field->options);
        if (is_array($value)) {
          foreach ($value as $k => $v) {
            if (!isset($options[$v])) {
              $value[$k] = '';
            }
          }
        }
        else {
          if (!isset($options[$value])) {
            $value = '';
          }
        }
      }

      $field->value = $value;
    }
  }

  protected function validateFields() {
    $empty_grids = array();
    foreach ($this->webform->fields as $name => $field) {
      if (!empty($field->grid_name)) {
        $empty_grids[$field->grid_name] = TRUE;
      }
    }

    foreach ($this->webform->fields as $name => $field) {
      if (!empty($field->grid_name)) {
        if (count($field->value) > 1) {
          unset($empty_grids[$field->grid_name]);
        }
        elseif (strpos($field->class, 'row-index') === FALSE) {
          foreach ($field->value as $value) {
            if (!empty($value)) {
              unset($empty_grids[$field->grid_name]);
              break;
            }
          }
        }
      }
    }

    foreach ($this->webform->fields as $name => $field) {
      // Ignore empty grids
      if (!empty($field->grid_name) && !empty($empty_grids[$field->grid_name])) {
        continue;
      }

      $values = is_array($field->value) ? $field->value : array($field->value);
      $error = array();

      foreach ($values as $key => $value) {
        // Check required
        if ($field->required) {
          if ($field->type == 'numeric') {
            if (empty($value) && $value !== '0') {
              $error[$key] = t('This field is required');
            }

          }
          elseif (empty($value)) {
            $error[$key] = t('This field is required');
          }

        }

        // Check length
        if ($field->required && strlen($value)) {
          if ($field->minlength && strlen($value) < $field->minlength) {
            $error[$key] = t('This field must have at least @length symbols', array('@length' => $field->minlength));
          }
          if ($field->maxlength && strlen($value) > $field->maxlength) {
            $error[$key] = t('This field must have not more than @length symbols', array('@length' => $field->maxlength));
          }
        }

        // Check date
        if ($field->type == 'date' && !empty($value)) {
          list($day, $month, $year) = explode('.', $value);
          $day = (int) ltrim(trim($day), '0');
          $month = (int) ltrim(trim($month), '0');
          $year = (int) $year;
          if ($year < date('Y') - 50 || $year > date('Y') + 50 || !checkdate($month, $day, $year)) {
            $error[$key] = t('Wrong field format: date needed');
          }
        }

        // Check fiscal period to corespond to webform's period
        if ($field->type == 'period') {
          $month = 0;
          $year = 0;

          if (!empty($value)) {
            $period = explode('/', $value);
            switch (strtoupper($period[0])) {
              case 'A':
                $month = 0;
                $year = $period[1];
                break;
              case 'L':
                $month = ltrim($period[1], ' 0') - 1;
                $year = $period[2];
                break;
              case 'T':
                $month = (ltrim($period[1], ' 0') - 1) * 3;
                $year = $period[2];
                break;
              case 'S':
                $month = (ltrim($period[1], ' 0') - 1) * 6;
                $year = $period[2];
                break;
              case 'F':
                $month = (ltrim($period[1], ' 0') - 1) * 4;
                $year = $period[2];
                break;
            }

            $period = ($year) ? mktime(0, 0, 0, $month + 1, 1, $year) : 0;
            $period_start = $this->webform->period_start ? $this->webform->period_start : mktime(0, 0, 0, 1, 1, date('Y') - 20);
            $period_end = $this->webform->period_end ? $this->webform->period_end : time();

            if ($period < $period_start || $period > $period_end) {
              $error[$key] = t('Wrong fiscal period');
            }
          }
        }

        // Check by pattern or sanitize values
        $pattern = empty($field->pattern) ? '' : $field->pattern;
		$err = empty($field->pattern) ? '' : t('Wrong field format: the pattern did not match');
        switch ($field->type) {
          case 'latintext':
            $pattern = "|^[a-z0-9,.-]*$|i";
            $err = t('Wrong field format: latin text needed');
            break;
          case 'numeric':
            $pattern = "|^([-][0-9]{1})?[0-9]*?$|i";
            $err = t('Wrong field format: natural number needed');
            break;
          case 'float':
            $pattern = "|^([-][0-9]{1})?[0-9]*([.,,][0-9]+)?$|i";
            $err = t('Wrong field format: float number needed');
            break;
          case 'period':
            $pattern = "@^(a\/\d{4}$)|(t\/[1234]\/\d{4}$)|(s\/[12]\/\d{4}$)|(l\/\d{1,2}\/\d{4})|(f\/[123]\/\d{4}$)$@i";
            $err = t('Wrong field format: fiscal period needed');
            break;
        }

        if (!empty($pattern) && !preg_match($pattern, $value)) {
          $error[$key] = $err;
        }
      }

      if (!empty($error)) {
        $field->error = is_array($field->value) ? $error : $error[0];
      }
    }
  }

  protected function validateAutofields() {
    $formulas = $this->GetAutofieldsFormulas();

    foreach ($formulas as $formula) {
      $check = '';

      $result_field = $formula['result'];
      if (preg_match('|(\{n\})$|i', $result_field)) {
        $result_field = str_replace(array('{n}', '{N}'), '', $result_field);
        $n = 1;
      }
      $result = $this->webform->fields[$result_field]->value;

      $current_decimal_length = !empty($this->webform->fields[$result_field]->decimal_length) ? $this->webform->fields[$result_field]->decimal_length : 2;

      if (is_array($result)) {
        foreach ($result as $key => $val) {
          $check = '';
          $tr = array();
          foreach ($formula['fields'] as $field) {
            $name = ltrim($field, '&');
            $value = $this->webform->fields[$name]->value[$key];
            $decimal_length = !empty($this->webform->fields[$name]->decimal_length) ? $this->webform->fields[$name]->decimal_length : 2;

            $value = empty($value) ? '0' : $value;
            $check .= '$' . $name . ' = ' . _mywebform_fix_money($value, $decimal_length) . ";\n";
            $tr['[' . $field . ']'] = '$' . $name;
          }

          $check .= 'return ' . strtr($formula['formula'], $tr) . ';';
          $check = eval($check);

          if (round($check, $current_decimal_length) != round($result[$key], $current_decimal_length)) {
            $this->webform->fields[$result_field]->warning[$key] = sprintf("%s (%1.{$current_decimal_length}f -> %1.{$current_decimal_length}f)", t($formula['message']), round($result[$key], $current_decimal_length), round($check, $current_decimal_length));
          }
        }
      }
      elseif (empty($n)) {
        $tr = array();
        foreach ($formula['fields'] as $field) {
          $name = ltrim($field, '&');
          $value = $this->webform->fields[$name]->value;
          $decimal_length = !empty($this->webform->fields[$name]->decimal_length) ? $this->webform->fields[$name]->decimal_length : 2;

          if (empty($this->webform->fields[$name]->grid_name)) {
            $value = empty($value) ? 0 : $value;
          }
          else {
            $value = empty($value) ? array() : $value;
          }
          if (is_array($value)) {
            $count_val = count($value);
            if($count_val) {
              $dec_length = array_fill(0, $count_val, $decimal_length);
            }
            else {
              $dec_length = array('2');
            }

            if($this->webform->fields[$name]->type == 'text') {
              foreach($value as $kk => $vv) {
                $value[$kk] = "'" . addslashes($vv) . "'";
              }
            }
            else {
              $value = array_map('_mywebform_fix_money', $value, $dec_length);
            }

            $check .= '$' . $name . ' = array(' . implode(', ', array_filter($value)) . ");\n";
          }
          else {
            if($this->webform->fields[$name]->type == 'text') {
              $value = "'" . addslashes($value) . "'";
            }
            else {
              $value = _mywebform_fix_money($value, $decimal_length);
            }
            $check .= '$' . $name . ' = ' . $value . ";\n";
          }
          $tr['[' . $field . ']'] = '$' . $name;
        }
        $check .= 'return ' . strtr($formula['formula'], $tr) . ';';

        $check = eval($check);

        if (round($check, $current_decimal_length) != round($result, $current_decimal_length)) {
          $this->webform->fields[$result_field]->warning = sprintf("%s (%1.{$current_decimal_length}f -> %1.{$current_decimal_length}f)", t($formula['message']), round($result, $current_decimal_length), round($check, $current_decimal_length));
        }
      }
    }
  }

  /**
   * Determines all the grids from the webform and the count of values
   *
   * @return array
   *   An associative array with keys as grid names and values as count of elements in grid
   */
  protected function GetGridList() {
    $grids = array();

    foreach ($this->webform->fields as $name => $field) {
      if (empty($field->grid_name)) {
        continue;
      }

      if (empty($grids[$field->grid_name])) {
        $grids[$field->grid_name] = array(
          'count' => 0,
          'fields' => array(),
        );
      }

      if (!$grids[$field->grid_name]['count']) {
        $grids[$field->grid_name]['count'] = is_array($field->value) ? count($field->value) : 0;
      }

      if (!in_array($field->name, $grids[$field->grid_name]['fields'])) {
        $grids[$field->grid_name]['fields'][] = $field->name;
      }
    }

    return $grids;
  }

  protected function BuildFormHtml($preview = FALSE) {
    _mywebform_render_fields(NULL, $this, $preview);

    if(!empty($_GET['print']) && !empty($this->webform->html_print_template)) {
      $content = preg_replace_callback("|@([^;]+);|i", '_mywebform_render_fields', $this->webform->html_print_template);
    }
    elseif($preview && !empty($this->webform->html_preview_template)) {
      $content = preg_replace_callback("|@([^;]+);|i", '_mywebform_render_fields', $this->webform->html_preview_template);
    }
    else {
      $content = preg_replace_callback("|@([^;]+);|i", '_mywebform_render_fields', $this->webform->html_template);
    }

    $content = preg_replace_callback("|\[\"([^\"]*)\"\]|i", '_mywebform_translate', $content);
    return $content;
  }

  protected function RenderAttachments() {
    $output = '';
    $files = array();

    if (!empty($this->webform->files['doc'])) {
      $output .= '<div class="attachments">';
      $output .= '<p><b>' . t('Attached documents') . ':</b></p>';
      $output .= '<table border="0" cellpadding="0" cellspacing="5">';
      $output .= '<tbody>';
      $attr = array(
        'html' => TRUE,
        'language' => language_default(),
        'attributes' => array(
          'class' => 'file',
          '_target' => 'blank',
        ),
      );

      foreach ($this->webform->files['doc'] as $file) {
        if (empty($file->locked)) {
          $path = mywebform_files_dir() . '/' . $file->filename;
        }
        else {
          $path = $file->filename;
        }
        if (file_exists($path)) {
          $title = explode('.', $file->filename);
          array_pop($title);
          $title = str_replace('_', ' ', implode('.', $title));
          $attr['attributes']['class'] = 'file';
          switch (file_get_mimetype($file->filename)) {
            case 'application/pdf':
              $attr['attributes']['class'] .= ' pdf';
              break;

            case 'application/msword':
              $attr['attributes']['class'] .= ' doc';
              break;

            case 'application/vnd.ms-excel':
            case 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet':
              $attr['attributes']['class'] .= ' xls';
              break;
          }
          $output .= '<tr>';
          $output .= '<td valign="top">';
          $output .= l('&nbsp;', $path, $attr);
          $output .= '</td>';
          $output .= '<td valign="top" width="100%">';
          $attr['attributes']['class'] = 'name';
          $output .= l($title, $path, $attr);
          $output .= '<br>' . t('Created') . ': <span style="color: rgb(110, 110, 110);">' . date("d/m/Y", filemtime($path)) . '</span>';
          $output .= '</td>';
          $output .= '</tr>';
        }
      }
      $output .= '</tbody>';
      $output .= '</table>';
      $output .= '</div>';
    }
    return $output;
  }

  protected function BuildAutofields() {
    $output = '';

    foreach ($this->webform->fields as $field) {
      $field->autofield = array();
    }

    $formulas = $this->GetAutofieldsFormulas();

    foreach ($formulas as $key => $formula) {
      $this->AutofieldsJsFunction($formulas[$key]);
    }

    foreach ($formulas as $formula) {
      foreach ($formula['fields'] as $name) {
        $name = ltrim($name, '&');
        if (isset($this->webform->fields[$name])) {
          $this->webform->fields[$name]->autofield[] = $formula['func_name'];
        }
      }
    }

    if (!empty($formulas)) {
      foreach ($formulas as $formula) {
        $output .= $formula['function'] . "\n";
      }

      $output .= "
webform.validators.validateAutofields = function() {
	var errors = [];
	var messages = '';
	var result = 0;
	var curres = 0;
	var N = 0;
			";

      foreach ($formulas as $formula) {
        $output .= $formula['validate'];
      }

      $output .= "
	if(messages != '') {
		if(!confirm('" . t('There are some autocalculation exceptions') . ":\\n\\n' + messages + '\\n" . t('Do you wish to continue?') . "')) {
			for(x in errors) {
				webform.errors.push(errors[x]);
			}
		} else {
			for(x in errors) {
				webform.warnings.push(errors[x]);
			}
		}
	}

	webform.validatorsStatus['validateAutofields'] = 1;
	validateWebform();
}";

      drupal_add_js($output, 'inline');
    }
  }

  protected function GetAutofieldsFormulas() {
    $formulas = array();
    $autofields = $this->ClearAutofieldsComments($this->webform->autofields);
    $autofields = array_filter(array_map('trim', explode(";", $autofields)));

    foreach ($autofields as $item) {
      $formula = array();

      if (preg_match_all("|(.*)=[ ]*\[([^]]*)\]msg\((.*)\)|i", $item, $matches)) {
        $formula['formula'] = $matches[1][0];
        $formula['result'] = $matches[2][0];
        $formula['message'] = $matches[3][0];
        if (preg_match_all("|\[([^]]*)\]|i", $formula['formula'], $matches)) {
          $formula['fields'] = array_values(array_unique($matches[1]));
        }
        else {
          $formula['fields'] = array();
        }
      }
      if (!empty($formula)) {
        $formulas[] = $formula;
      }
    }

    return $formulas;
  }

  protected function ClearAutofieldsComments($autofields) {
    $rows = array_map('trim', explode("\n", $autofields));
    foreach ($rows as $key => $row) {
      $i = strpos($row, '//');
      if ($i !== FALSE) {
        $rows[$key] = trim(substr($row, 0, $i));
      }
    }

    return implode("\n", array_filter($rows));
  }

  protected function AutofieldsJsFunction(&$formula) {
    $decimal_length = 0;
    $result = $formula['result'];
    $by_row_index = FALSE;
    if (preg_match('|(\{n\})$|i', $result)) {
      $result = str_replace(array('{n}', '{N}'), '', $result);
      $by_row_index = TRUE;
    }
    $func_name = str_replace('-', '_', 'update_' . $result);
    $call = array();

    if (!empty($this->webform->fields[$result])) {
      $this->webform->fields[$result]->class .= ' autofield';
      $decimal_length = $this->webform->fields[$result]->decimal_length;
    }

    if (empty($decimal_length)) {
      $decimal_length = 2;
    }

    $validate = '';
    $function = "function " . $func_name . "(elem) {\n";
    if ($by_row_index) {
      $function .= "	var N = parseInt(elem.attr('row-index'));\n";
    }

    $n = $by_row_index ? "-'+N" : "'";
    $n_arg = $by_row_index ? "[N-1]" : "";
    $arg_tr = array();
    foreach ($formula['fields'] as $key => $field) {
      if (substr($field, 0, 1) == '&') {
        $field = substr($field, 1);
        $arg_tr['[&' . $field . ']'] = sprintf("Drupal.settings.mywebform.values.%s%s", $field, $n_arg);
      }
      else {
        $arg_tr['[' . $field . ']'] = sprintf("toFloat(Drupal.settings.mywebform.values.%s%s)", $field, $n_arg);
      }
    }

    $str = strtr($formula['formula'], $arg_tr);
    $function .= "	var result = " . $str . ";\n";
    $function .= sprintf("	$('#%s%s).val(formatNumber(result, %d)).trigger('change');\n", $result, $n, $decimal_length);
    $function .= "}\n";

    if ($by_row_index) {
      $validate .= sprintf("	for(N in Drupal.settings.mywebform.values.%s) {\n", $result);
      $validate .= "	N++;";
    }
    else {
      $validate .= "\n	N = 0;\n";
    }
    $validate .= "	result = " . $str . ";\n";
    $validate .= sprintf("	curres = toFloat(Drupal.settings.mywebform.values.%s%s);\n", $result, $n_arg);

    $validate .= "	if(formatNumber(curres, {$decimal_length}) != formatNumber(result, {$decimal_length})) {\n";
    $validate .= "	    var msg = '" . t($formula['message']) . " (' + formatNumber(curres, {$decimal_length}) + ' -> ' + formatNumber(result, {$decimal_length}) + ')';\n";
    $validate .= "		messages += msg + '\\n';\n";
    $validate .= "		errors.push({'fieldName' : '" . $result . "', 'index' : N - 1, 'msg' : msg});\n";
    $validate .= "	}\n";
    if ($by_row_index) {
      $validate .= "	}\n";
    }
    $formula['validate'] = $validate;
    $formula['function'] = $function;
    $formula['func_name'] = $func_name;
    $formula['call'] = implode('', $call);
  }

  protected function fixDateValue($value) {
    $value = str_replace('/', '.', $value);
    $value = str_replace('-', '.', $value);

    list($day, $month, $year) = explode('.', $value);
    $day = (int) ltrim(trim($day), '0');
    $month = (int) ltrim(trim($month), '0');
    $year = (int) $year;

    if (checkdate($month, $day, $year)) {
      $value = sprintf("%02d.%02d.%d", $day, $month, $year);
    }
    else {
      //Check if format is YYYY-MM-DD
      list($year, $month, $day) = explode('.', $value);
      $month = (int) ltrim(trim($month), '0');
      $day = (int) ltrim(trim($day), '0');
      $year = (int) $year;

      if (checkdate($month, $day, $year)) {
        $value = sprintf("%02d.%02d.%d", $day, $month, $year);
      }
    }

    return $value;
  }
}

class WebFormException extends Exception {
}

function mywebform_sanitize_xml($xml, $tech_name) {
  try {
    $form = new WebForm($tech_name, $xml);
    return $form->OutputXML();
  } catch (Exception $e) {
  }

  return $xml;
}

function _mywebform_render_fields($matches, $form = NULL, $preview = FALSE) {
  static $set_form = NULL;
  static $set_preview = FALSE;

  if (isset($form)) {
    $set_form = $form;
    $set_preview = $preview;
    return;
  }

  $index = explode('[', str_replace(']', '', $matches[1]));
  $field_name = array_shift($index);

  return $set_form->RenderField($field_name, array_shift($index), $set_preview);
}

function _mywebform_translate($matches) {
  if (!empty($matches[1])) {
    return mywebform_t($matches[1]);
  }
  return '';
}

/**
 * Saves an uploaded file from PHP temp directory to specified one and return a string
 * with information about it
 *
 * @param array $file - an array being a piece of $_FILES for the specific one file
 * @return string - a string with information about the uploaded file in format CHECKSUM;FID;URL
 */
function _mywebform_attach_file($file, $allowed_mime = array(), $max_size = 0, $extensions = 'jpg jpeg gif png txt html doc xls pdf ppt pps odt ods odp') {
  global $user;

  $files = array();
  $result = array();

  // Normalize value
  if (is_array($file['name'])) {
    $i = 1;
    foreach ($file['name'] as $key => $value) {
      $files[$i++] = array(
        'name' => $file['name'][$key],
        'type' => $file['type'][$key],
        'tmp_name' => $file['tmp_name'][$key],
        'error' => $file['error'][$key],
        'size' => $file['size'][$key],
      );
    }
  }
  else {
    $files[1] = $file;
  }

  // Upload files and fill results
  foreach ($files as $key => $f) {
    if (empty($f['name'])) {
      $result[$key] = '';
      continue;
    }
    if ($f['error'] != 0) {
      $result[$key] = '';
      continue;
    }
    if (!empty($allowed_mime) && !in_array($f['type'], $allowed_mime)) {
      $result[$key] = '';
      continue;
    }
    if ($max_size && $f['size'] >= $max_size) {
      $result[$key] = '';
      continue;
    }
    $regex = '/\.('. ereg_replace(' +', '|', preg_quote($extensions)) .')$/i';
    if (!preg_match($regex, $f['name'])) {
      $result[$key] = '';
      continue;
    }

    $obj = new stdClass();
    $obj->uid = $user->uid;
    $obj->filename = $f['name'];
    $obj->filemime = file_get_mimetype($obj->filename);
    $obj->filesize = $f['size'];

    $name = explode('.', $f['name']);
    $ext = end($name);
    $filepath = file_directory_path() . '/attachments';
    file_check_directory($filepath, FILE_CREATE_DIRECTORY);
    do {
      $name = time() . '-' . md5(uniqid(mt_rand(), TRUE)) . '.' . $ext;
      $obj->filepath = $filepath . '/' . $name;
    } while (file_exists($obj->filepath));

    if (!move_uploaded_file($f['tmp_name'], $obj->filepath)) {
      $result[$key] = '';
      continue;
    }

    $obj->status = FILE_STATUS_PERMANENT;
    $obj->timestamp = time();
    drupal_write_record('files', $obj);

    $md5 = md5_file($obj->filepath);
    $url = url('getfile', array(
      'absolute' => TRUE,
      'language' => language_default,
      'query' => array('fid' => $obj->fid)
    ));
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
  $file = db_fetch_object(db_query("SELECT * FROM {files} WHERE fid = %d ", $fid));
  db_query("DELETE FROM {files} WHERE fid = %d ", $fid);
  if (!empty($file)) {
    file_delete($file->filepath);
  }
}

function _mywebform_complex_set_value(&$complex, $path, $value) {
  if (!count($path)) {
    $complex['#value'] = $value;
  }
  else {
    $current = array_shift($path);
    if (!isset($complex[$current])) {
      $complex[$current] = array();
    }
    _mywebform_complex_set_value($complex[$current], $path, $value);
  }
}

function _mywebform_complex_to_xml($complex, $step = 0) {
  $xml = '';

  foreach ($complex as $key => $value) {
    if (is_array($value)) {
      $attr = '';
      $inner = '';
      $a = explode(' ', $key);
      $key_close = $a[0];

      if (isset($value['#attr'])) {
        $attr = $value['#attr'];
        unset($value['#attr']);
        if (is_array($attr)) {
          foreach ($attr as $n => $v) {
            $attr[$n] = $n . '="' . $v . '"';
          }
          $attr = trim(implode(' ', $attr));
        }
        if (!empty($attr)) {
          $attr = ' ' . $attr;
        }
      }
      if (isset($value['#value'])) {
        $inner = $value['#value'];
        unset($value['#value']);
      }
      if (empty($value)) {
        if (!strlen((string) $inner)) {
          $xml .= sprintf("<%s%s />", $key, $attr);
        }
        else {
          $xml .= sprintf("<%s%s>%s</%s>", $key, $attr, $inner, $key_close);
        }
      }
      else {
        $xml .= sprintf("<%s%s>%s%s</%s>", $key, $attr, $inner, _mywebform_complex_to_xml($value, $step + 1), $key_close);
      }
    }
  }

  return $xml;
}

function _mywebform_check_cyr_with_latin($str) {
  if (preg_match("/[а-я]+/ui", $str)) {
    if (preg_match("/[a-z]+/ui", $str)) {
      return TRUE;
    }
  }
  return FALSE;
}

function _mywebform_fix_cyr_with_latin($str) {
  static $msg = FALSE;

  if (preg_match("/[а-я]+/ui", $str)) {
    if (preg_match("/[a-z]+/ui", $str)) {
      $tr = array(
        'А' => 'A', 'В' => 'B', 'Е' => 'E', 'З' => '3', 'К' => 'K', 'М' => 'M', 'Н' => 'H', 'О' => 'O',
        'Р' => 'P', 'С' => 'C', 'Т' => 'T', 'У' => 'Y', 'Х' => 'X', 'Я' => 'R', 'а' => 'a', 'в' => 'b', 'е' => 'e',
        'з' => '3', 'к' => 'k', 'м' => 'm', 'н' => 'h', 'о' => 'o', 'р' => 'p', 'с' => 'c', 'т' => 't', 'у' => 'y',
        'х' => 'x', 'г' => 'r',
      );
      if (!$msg) {
        drupal_set_message(t('The declaration contained wrong cyrilic values. The content has been fixed. Check it once more to ensure that it has no more mistakes!!!'));
        $msg = TRUE;
      }
      return strtr($str, $tr);
    }
  }
  return $str;
}

function sum($arr) {
  if (is_array($arr)) {
    return array_sum($arr);
  }
  return $arr;
}

function positive_only($value) {
  return $value > 0 ? $value : 0;
}

function _mywebform_fix_money($value, $decimal_length = 2) {
  return round(str_replace(',', '.', $value), $decimal_length);
}

function row_count($values) {
  $count = 0;
  if(is_array($values)) {
    foreach($values as $k => $v) {
      if($v !== '') {
        $count++;
      }
    }
  }
  return $count;
}

function mywebform_upload_grid_xml_form(&$form_state, $webform) {
  $form = array();

  $form['#webform'] = $webform;
  $form['#attributes'] = array('enctype' => 'multipart/form-data');

  $form['warning'] = array(
    '#value' => t('Warning: Your unsaved changes will be lost! Please, save your changes befor uploadning grid data!'),
    '#prefix' => '<div class="message warning">',
    '#suffix' => '</div>',
  );

  $form['grid_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Grid name'),
    '#prefix' => '<div style="display: none;">',
    '#suffix' => '</div>',
  );

  $form['xml_file'] = array(
    '#type' => 'file',
    '#title' => t('Upload XML'),
    '#size' => 20,
  );

  $form['actions'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );

  $form['actions']['cancel'] = array(
    '#value' => '<a class="cancel-button" href="#">' . t('Cancel') . '</a>',
  );

  return $form;
}

function mywebform_upload_grid_xml_form_submit($form, &$form_state) {
  $webform = $form['#webform'];

  $file = file_save_upload('xml_file');
  if (!empty($file->fid)) {
    db_query("DELETE FROM {files} WHERE fid = %d", $file->fid);

    if ($file->filemime == 'application/xml') {
      $xml = file_get_contents($file->filepath);

      if (!empty($xml)) {
        $webform->uploaded_xml = array(
          'xml' => $xml,
          'grid_name' => $form_state['values']['grid_name'],
        );
        $form_state['rebuild'] = TRUE;
      }
    }
    file_delete($file->filepath);
  }
}
