<?php

require_once 'mywebform.private.inc';

function mywebform_type_edit_form($form_state, $webform = NULL, $add = TRUE) {
  $form = array();
  
  if (!$add && empty($webform)) {
    drupal_not_found();
    exit();
  }
  
  if ($add) {
    $webform = (object) array(
      'tech_name' => '',
      'name' => '',
      'title' => '',
      'tid' => '',
      'html_template' => '',
      'autofields' => '',
      'status' => '',
      'period_start' => '',
      'period_end' => '',
    );
    drupal_set_title(t('Add new webform'));
  }
  else {
    drupal_set_title(t('Edit webform @name', array('@name' => $webform->name)));
  }
  
  // JS Calendar
  global $language;
  drupal_add_css('misc/jscal2/css/jscal2.css');
  drupal_add_css('misc/jscal2/css/border-radius.css');
  drupal_add_css('misc/jscal2/css/steel/steel.css');
  drupal_add_js('misc/jscal2/js/jscal2.js');
  drupal_add_js('misc/jscal2/js/lang/' . $language->language . '.js');
  drupal_add_js(drupal_get_path('module', 'mywebform') . '/mywebform-admin.js');
  drupal_add_css(drupal_get_path('module', 'mywebform') . '/mywebform.css');
  
  $form['#attributes'] = array('enctype' => 'multipart/form-data');
  $form['#webform'] = $webform;
  $form['#aux'] = array();
  
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  
  $form['general']['tech_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Technical name'),
    '#description' => t('Machine readable technical name. It is used to identify the webform by its name. Must be unique and lowercase.'),
    '#default_value' => $webform->tech_name,
    '#required' => TRUE,
  );
  
  $form['general']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('A short name for the webform.'),
    '#default_value' => $webform->name,
    '#required' => TRUE,
  );
  
  $form['general']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('A long title for the webform.'),
    '#default_value' => $webform->title,
    '#maxlength' => 255,
    '#required' => TRUE,
  );
  
  $form['general']['tid'] = array(
    '#type' => 'select',
    '#title' => t('Category'),
    '#description' => t('A category, the webform belongs to.'),
    '#options' => _mywebform_term_list(variable_get('mywebform_vid', 0), t('- None -')),
    '#default_value' => $webform->tid,
  );
  
  $form['general']['html_template'] = array(
    '#type' => 'textarea',
    '#title' => t('HTML template'),
    '#rows' => 4,
    '#required' => TRUE,
    '#default_value' => $webform->html_template,
  );
  
  $form['general']['autofields'] = array(
    '#type' => 'textarea',
    '#title' => t('Autocalculate expressions'),
    '#rows' => 4,
    '#default_value' => $webform->autofields,
  );
  
  $form['general']['status'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#options' => array(
      '1' => t('Enabled'),
      '0' => t('Disabled'),
    ),
    '#default_value' => $webform->status,
    '#required' => TRUE,
  );
  
  $form['general']['period_start'] = array(
    '#type' => 'textfield',
    '#title' => t('Period start'),
    '#size' => 10,
    '#attributes' => array(
      'class' => 'date',
    ),
    '#default_value' => empty($webform->period_start) ? '' : date('d.m.Y', $webform->period_start),
  );
  
  $form['general']['period_end'] = array(
    '#type' => 'textfield',
    '#title' => t('Period end'),
    '#size' => 10,
    '#attributes' => array(
      'class' => 'date',
    ),
    '#default_value' => empty($webform->period_end) ? '' : date('d.m.Y', $webform->period_end),
  );
  
  $form['general']['may_upload_grid'] = array(
    '#type' => 'checkbox',
    '#title' => t('Upload grids data from XML'),
    '#default_value' => $webform->may_upload_grid,
  );
  
  $form['files'] = array(
    '#type' => 'fieldset',
    '#title' => t('Files'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  
  $form['files']['js'] = _mywebform_file_item('js_file', t('JavaScript'), t('JavaScript file'), $webform->files['js'][0]);
  $form['files']['css'] = _mywebform_file_item('css_file', t('CSS'), t('CSS file'), $webform->files['css'][0]);
  $form['files']['xsd'] = _mywebform_file_item('xsd_file', t('XSD'), t('XSD file'), $webform->files['xsd'][0]);
  for ($i = 1; $i <= variable_get('mywebform_attachments_amount', 3); $i++) {
    $form['files']['doc' . $i] = _mywebform_file_item('doc_file' . $i, t('Document'), t('Document file'), $webform->files['doc'][$i - 1]);
  }
  
  $form['actions'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  
  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  
  if (!$add) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
    );
  }
  
  $form['actions']['cancel'] = array(
    '#value' => l(t('Cancel'), $webform->id ? 'admin/build/mywebform/' . $webform->id : 'admin/build/webform'),
  );
  
  return $form;
}

function mywebform_type_edit_form_validate($form, &$form_state) {
  if (!empty($form_state['values']['period_start'])) {
    list($day, $month, $year) = explode('.', trim($form_state['values']['period_start']));
    $from = mktime(0, 0, 0, ltrim($month, ' 0'), ltrim($day, ' 0'), $year);
  }
  
  if (!empty($form_state['values']['period_end'])) {
    list($day, $month, $year) = explode('.', trim($form_state['values']['period_end']));
    $to = mktime(0, 0, 0, ltrim($month, ' 0'), ltrim($day, ' 0'), $year);
  }
  
  if ($from && $to && $from > $to) {
    form_set_error('period_start', t('Starting date date must be lower than end date'));
  }
}

function mywebform_type_edit_form_submit($form, &$form_state) {
  $obj = $form['#webform'];
  
  if ($form_state['values']['op'] == t('Delete')) {
    drupal_goto('admin/build/mywebform/' . $obj->id . '/delete');
  }
  
  // General object
  $obj->tid = $form_state['values']['tid'];
  $obj->tech_name = $form_state['values']['tech_name'];
  $obj->name = $form_state['values']['name'];
  $obj->title = $form_state['values']['title'];
  $obj->html_template = $form_state['values']['html_template'];
  $obj->autofields = $form_state['values']['autofields'];
  $obj->status = $form_state['values']['status'];
  $obj->may_upload_grid = $form_state['values']['may_upload_grid'];
  if (!empty($form_state['values']['period_start'])) {
    list($day, $month, $year) = explode('.', trim($form_state['values']['period_start']));
    $obj->period_start = mktime(0, 0, 0, ltrim($month, ' 0'), ltrim($day, ' 0'), $year);
  }
  else {
    $obj->period_start = 0;
  }
  if (!empty($form_state['values']['period_end'])) {
    list($day, $month, $year) = explode('.', trim($form_state['values']['period_end']));
    $obj->period_end = mktime(0, 0, 0, ltrim($month, ' 0'), ltrim($day, ' 0'), $year);
  }
  else {
    $obj->period_end = 0;
  }
  // Set aux values
  if (empty($obj->aux)) {
    $obj->aux = array();
  }
  if (is_array($form['#aux'])) {
    foreach ($form['#aux'] as $item) {
      if (is_array($form_state['values'][$item])) {
        $obj->aux[$item] = implode(', ', array_filter($form_state['values'][$item]));
      }
      else {
        $obj->aux[$item] = $form_state['values'][$item];
      }
    }
  }
  
  $obj->files['js'][0] = _mywebform_get_uploaded_file($form_state, 'js_file', 'js', $obj->files['js'][0]);
  $obj->files['css'][0] = _mywebform_get_uploaded_file($form_state, 'css_file', 'css', $obj->files['css'][0]);
  $obj->files['xsd'][0] = _mywebform_get_uploaded_file($form_state, 'xsd_file', 'xsd', $obj->files['xsd'][0]);
  
  for ($i = 1; $i <= variable_get('mywebform_attachments_amount', 3); $i++) {
    $obj->files['doc'][$i - 1] = _mywebform_get_uploaded_file($form_state, 'doc_file' . $i, 'pdf doc xls xlsx', $obj->files['doc'][$i - 1]);
  }
  
  if (mywebform_save($obj)) {
    t($obj->title); // Force introduce string in locales_sources table
    drupal_set_message(t('The webform <em>@name</em> has been saved', array('@name' => $obj->name)));
    watchdog('mywebform', 'Webform @name saved', array('@name' => $obj->name));
    $form_state['redirect'] = 'admin/build/mywebform/' . $obj->id;
  }
  else {
    drupal_set_message(t('The webform <em>@name</em> save failed', array('@name' => $obj->name)), 'error');
    watchdog('mywebform', 'Webform @name save failed', array('@name' => $obj->name), WATCHDOG_ERROR);
    $form_state['redirect'] = 'admin/build/mywebform';
  }
}

function mywebform_type_delete_form($form_state, $webform = NULL) {
  $form = array();
  
  if (empty($webform)) {
    drupal_not_found();
    exit();
  }
  
  $form['#webform'] = $webform;
  $question = t('Do you really want to delete the webform <em>@name</em>?', array('@name' => $webform->name));
  return confirm_form($form, $question, 'admin/build/mywebform/' . $webform->id . '/edit');
}

function mywebform_type_delete_form_submit($form, &$form_state) {
  $obj = $form['#webform'];
  
  // Remove attached files
  foreach ($obj->files as $collection) {
    foreach ($collection as $file) {
      if (!empty($file->filename)) {
        file_delete(mywebform_files_dir() . '/' . $file->filename);
      }
      if (!empty($file->id)) {
        db_query("DELETE FROM {mywebform_files} WHERE id = %d ", $file->id);
      }
    }
  }
  
  // Remove fields
  db_query("DELETE FROM {mywebform_fields} WHERE mywebform_id = %d ", $obj->id);
  
  // Remove form
  db_query("DELETE FROM {mywebforms} WHERE id = %d ", $obj->id);
  
  // Throw a message
  drupal_set_message(t('The webform <em>@name</em> has been deleted', array('@name' => $obj->name)));
  watchdog('mywebform', 'Webform @name deleted', array('@name' => $obj->name));
  $form_state['redirect'] = 'admin/build/mywebform';
}

function mywebform_field_edit_form($form_state, $field = NULL, $webform = NULL, $add = TRUE) {
  $form = array();
  
  if ($add && empty($webform)) {
    drupal_not_found();
    exit();
  }
  
  if (!$add && empty($field)) {
    drupal_not_found();
    exit();
  }
  
  if (!$add) {
    $form['#field'] = $field;
    drupal_set_title(t('Edit field @name', array('@name' => $field->name)));
  }
  else {
    $field = new stdClass();
    $form['#webform'] = $webform;
    drupal_set_title(t('Add new field for webform @name', array('@name' => $webform->name)));
  }
  
  drupal_add_js("
Drupal.behaviors.mywebform = function() {
  $('#edit-type').change(function () {
    var val = $(this).val();
    $('#edit-format-wrapper').css('display', val == 'custom' ? 'block' : 'none');
    $('#edit-decimal-length-wrapper').css('display', val == 'float' ? 'block' : 'none');
  }).trigger('change');
  
  $('#edit-widget').change(function () {
    var val = $(this).val();
    $('#edit-rows-wrapper').css('display', val == 'textarea' ? 'block' : 'none');
    $('#edit-options-wrapper').css('display', (val == 'select' || val == 'container') ? 'block' : 'none');
    $('#edit-container-wrapper').css('display', (val == 'select') ? 'block' : 'none');
  }).trigger('change');
}
  ", 'inline');
  
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  
  $form['general']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#required' => TRUE,
    '#default_value' => $field->name,
  );
  
  $form['general']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $field->title,
  );
  
  $form['general']['xpath'] = array(
    '#type' => 'textfield',
    '#title' => t('XPath'),
    '#default_value' => $field->xpath,
  );
  
  $form['general']['type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#options' => _mywebform_get_field_types(),
    '#required' => TRUE,
    '#default_value' => $field->type,
  );
  
  $form['general']['format'] = array(
    '#type' => 'textfield',
    '#title' => t('Format'),
    '#default_value' => $field->format,
  );
  
  $form['general']['widget'] = array(
    '#type' => 'select',
    '#title' => t('Widget'),
    '#options' => _mywebform_get_field_widgets(),
    '#required' => TRUE,
    '#default_value' => $field->widget,
  );
  
  $form['general']['grid_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Grid name'),
    '#description' => t("If this element is dynamically generated within a grid, write its grid name here"),
    '#default_value' => $field->grid_name,
  );
  
  $form['general']['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#description' => t("Is used to order the nodes in the result XML file after submit the webform"),
    '#delta' => 100,
    '#default_value' => !empty($field->weight) ? $field->weight : 0,
  );
  
  $form['general']['status'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#options' => array(
      '1' => t('Enabled'),
      '0' => t('Disabled'),
    ),
    '#required' => TRUE,
    '#default_value' => $field->status,
  );
  
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  
  $form['options']['container'] = array(
    '#type' => 'textfield',
    '#title' => t('Container'),
    '#description' => t("Container variable's name to store a single copy of values. Use it only for grid."),
    '#default_value' => $field->container,
  );
  
  $form['options']['ignore_empty'] = array(
    '#type' => 'checkbox',
    '#title' => t('Do not include in XML if is empty'),
    '#default_value' => $field->ignore_empty,
  );
  
  $form['options']['export_result'] = array(
    '#type' => 'checkbox',
    '#title' => t('Export value to resultset'),
    '#default_value' => $field->export_result,
  );
  
  $form['options']['force_def_val'] = array(
    '#type' => 'checkbox',
    '#title' => t('Force value from default'),
    '#default_value' => $field->force_def_val,
  );
  
  $form['options']['class'] = array(
    '#type' => 'textfield',
    '#title' => t('Class'),
    '#default_value' => $field->class,
  );
  
  $form['options']['default_value'] = array(
    '#type' => 'textfield',
    '#title' => t('Default value'),
    '#autocomplete_path' => 'mywebform/predefined/autocomplete',
    '#default_value' => $field->default_value,
  );
  
  $form['options']['options'] = array(
    '#type' => 'textarea',
    '#title' => t('Options'),
    '#description' => t('SQL query or PHP code encapsulated in <?php ?>, or list of key;;value separated by line breaks'),
    '#rows' => 4,
    '#default_value' => $field->options,
  );
  
  $form['options']['attributes'] = array(
    '#type' => 'textarea',
    '#title' => t('Attributes'),
    '#description' => t('Additional HTML attributes for this element, list of key;;value separated by line breaks'),
    '#rows' => 4,
    '#default_value' => $field->attributes,
  );
  
  $form['options']['rows'] = array(
    '#type' => 'textfield',
    '#title' => t('Rows'),
    '#default_value' => $field->rows,
  );
  
  $form['options']['disabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disabled'),
    '#default_value' => $field->disabled,
  );
  
  $form['options']['readonly'] = array(
    '#type' => 'checkbox',
    '#title' => t('Readonly'),
    '#default_value' => $field->readonly,
  );
  
  $form['options']['required'] = array(
    '#type' => 'checkbox',
    '#title' => t('Required'),
    '#default_value' => $field->required,
  );
  
  $form['validate'] = array(
    '#type' => 'fieldset',
    '#title' => t('Validate'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  
  $form['validate']['minlength'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum length'),
    '#default_value' => $field->minlength,
  );
  
  $form['validate']['maxlength'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum length'),
    '#default_value' => $field->maxlength,
  );
  
  $form['validate']['decimal_length'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum length of decimal part'),
    '#default_value' => $field->decimal_length,
  );
  
  $form['validate']['pattern'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern'),
    '#default_value' => $field->pattern,
  );
  
  $form['actions'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  
  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  
  if(!$add) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
    );
  }
  
  $form['actions']['cancel'] = array(
    '#value' => l(t('Cancel'), 'admin/build/mywebform/' . ($field->mywebform_id ? $field->mywebform_id : $webform->id)),
  );
  
  return $form;
}

function mywebform_field_edit_form_submit($form, &$form_state) {
  $obj = $form['#field'] ? $form['#field'] : new stdClass();
  
  if ($form_state['values']['op'] == t('Delete')) {
    drupal_goto('admin/build/mywebform/field/' . $obj->id . '/delete');
  }
  
  if ($form['#webform']) {
    $obj->mywebform_id = $form['#webform']->id;
  }
  
  $obj->name = $form_state['values']['name'];
  $obj->title = $form_state['values']['title'];
  $obj->xpath = $form_state['values']['xpath'];
  $obj->grid_name = $form_state['values']['grid_name'];
  $obj->type = $form_state['values']['type'];
  $obj->widget = $form_state['values']['widget'];
  $obj->container = $form_state['values']['container'];
  $obj->format = $form_state['values']['format'];
  $obj->ignore_empty = $form_state['values']['ignore_empty'];
  $obj->export_result = $form_state['values']['export_result'];
  $obj->force_def_val = $form_state['values']['force_def_val'];
  $obj->disabled = $form_state['values']['disabled'];
  $obj->readonly = $form_state['values']['readonly'];
  $obj->required = $form_state['values']['required'];
  $obj->weight = $form_state['values']['weight'];
  $obj->default_value = $form_state['values']['default_value'];
  $obj->options = $form_state['values']['options'];
  $obj->attributes = $form_state['values']['attributes'];
  $obj->class = $form_state['values']['class'];
  $obj->rows = $form_state['values']['rows'];
  $obj->minlength = $form_state['values']['minlength'];
  $obj->maxlength = $form_state['values']['maxlength'];
  $obj->decimal_length = $form_state['values']['decimal_length'];
  $obj->pattern = $form_state['values']['pattern'];
  $obj->status = $form_state['values']['status'];
  
  if (empty($obj->aux)) {
    $obj->aux = array();
  }
  $obj->aux = serialize($obj->aux);
  
  if ($obj->id) {
    $result = drupal_write_record('mywebform_fields', $obj, 'id');
  }
  else {
    $obj->created = time();
    $result = drupal_write_record('mywebform_fields', $obj);
  }
  
  $webform = mywebform_load($obj->mywebform_id);
  if ($result) {
    drupal_set_message(t('The field <em>@name</em> has been saved', array('@name' => $obj->name)));
    watchdog('mywebform', 'Field @name saved for webform @webform',
      array('@name' => $obj->name, '@webform' => $webform->tech_name));
  }
  else {
    drupal_set_message(t('The field <em>@name</em> save failed', array('@name' => $obj->name)), 'error');
    watchdog('mywebform', 'Field @name save failed for webform @webform',
      array('@name' => $obj->name, '@webform' => $webform->tech_name), WATCHDOG_ERROR);
  }
  
  $form_state['redirect'] = 'admin/build/mywebform/' . $obj->mywebform_id;
}

function mywebform_field_delete_form($form_state, $field = NULL) {
  $form = array();
  
  if (empty($field)) {
    drupal_not_found();
    exit();
  }
  
  $form['#field'] = $field;
  $question = t('Do you really want to delete the field <em>@name</em>?', array('@name' => $field->name));
  return confirm_form($form, $question, 'admin/build/mywebform/field/' . $field->id . '/edit');
}

function mywebform_field_delete_form_submit($form, &$form_state) {
  $obj = $form['#field'];
  $webform = mywebform_load($obj->mywebform_id);
  
  db_query("DELETE FROM {mywebform_fields} WHERE id = %d ", $obj->id);
  drupal_set_message(t('The field <em>@name</em> has been deleted', array('@name' => $obj->name)));
  watchdog('mywebform', 'Field @name from webform @webform deleted', array('@name' => $obj->name, '@webform' => $webform->tech_name));
  $form_state['redirect'] = 'admin/build/mywebform/' . $obj->mywebform_id;
}

function mywebform_import_fields_form($form_state, $webform) {
  $form = array();
  
  if (empty($webform)) {
    drupal_not_found();
    exit();
  }
  
  $form['#attributes'] = array('enctype' => 'multipart/form-data');
  $form['#webform'] = $webform;

  $form['import_file'] = array(
    '#type' => 'file',
    '#title' => t('Import file'),
    '#description' => t('Allowed extensions: @ext', array('@ext' => 'xls')),
  );
  
  $form['import'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );
  
  return $form;
}

function mywebform_import_fields_form_submit($form, &$form_state) {
  set_time_limit(0);
  
  $webform = $form['#webform'];
  $file = file_save_upload('import_file', array('file_validate_extensions' => array('xls')));
  if (is_object($file)) {
    // Get XLS
    $rows = php_excel_load_file($file->filepath);
    
    // Remove file
    db_query("DELETE FROM {files} WHERE fid = %d", $file->fid);
    file_delete($file->filepath);
    
    $header = array_shift($rows);
    // Remove empty columns from the end of the array
    while (1) {
      $value = array_pop($header);
      if (!is_null($value)) {
        $header[] = $value;
        break;
      }
    }
    
    // Parse table as fields
    $fields = array();
    foreach ($rows as $row) {
      if (count(array_filter($row))) {
        if (count($row) == count($header)) {
          $fields[] = array_combine($header, $row);
        }
        else {
          $f = array();
          foreach ($header as $index => $key) {
            $f[$key] = $row[$index];
          }
          $fields[] = $f;
        }
      }
    }
    
    // Get current fields ids
    $result = db_query("SELECT id, name FROM {mywebform_fields} WHERE mywebform_id = %d ", $webform->id);
    while ($data = db_fetch_object($result)) {
      $field_ids[$data->name] = $data->id;
    }
    
    // Save fields
    foreach ($fields as $field) {
      if (empty($field['name'])) {
        continue;
      }
      
      if (!$field['id'] && !empty($field_ids[$field['name']])) {
        $field['id'] = $field_ids[$field['name']];
      }
      $field['mywebform_id'] = $webform->id;
      
      if ($field['id']) {
        if (drupal_write_record('mywebform_fields', $field, 'id')) {
          drupal_set_message(t('Field <em>@name</em> updated', array('@name' => $field['name'])));
        }
        else {
          drupal_set_message(t('Field <em>@name</em> update failed', array('@name' => $field['name'])), 'error');
        }
      }
      else {
        $field['created'] = time();
        if (drupal_write_record('mywebform_fields', $field)) {
          drupal_set_message(t('Field <em>@name</em> insert', array('@name' => $field['name'])));
        }
        else {
          drupal_set_message(t('Field <em>@name</em> insert failed', array('@name' => $field['name'])), 'error');
        }
      }
    }
    
    drupal_set_message(t('The fields have been imported successfuly'));
    return;
  }
  
  drupal_set_message(t('Failed to import fields'), 'error');
}
