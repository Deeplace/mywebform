<?php

require_once 'mywebform.private.inc';

function mywebform_settings_form($form_state) {
  $form = array();
  $weight = -100;
  
  $form['mywebform_attachments_amount'] = array(
    '#type' => 'select',
    '#title' => t('Maximal amount of attachments'),
    '#options' => drupal_map_assoc(range(1, 10)),
    '#default_value' => variable_get('mywebform_attachments_amount', 3),
    '#weight' => $weight++,
  );
  
  $vocabularies = array();
  foreach (taxonomy_get_vocabularies() as $vid => $voc) {
    $vocabularies[$vid] = $voc->name;
  }
  
  $form['mywebform_vid'] = array(
    '#type' => 'select',
    '#title' => t('Attached vocabulary'),
    '#options' => $vocabularies,
    '#default_value' => variable_get('mywebform_vid', 0),
    '#weight' => $weight++,
  );
  
  $predefined = mywebform_predefined_values();
  foreach ($predefined as $key => $value) {
    $predefined[$key] = $key . ' = ' . $value;
  }
  
  $form['predefined_values'] = array(
    '#type' => 'item',
    '#title' => t('Predefined values'),
    '#value' => theme('item_list', $predefined),
    '#weight' => $weight++,
  );
  
  $form['mywebform_user_defined_values'] = array(
    '#type' => 'textarea',
    '#title' => t('User defined values'),
    '#description' => t('Each pair must be in a separated line. Format: @varname;;value'),
    '#default_value' => variable_get('mywebform_user_defined_values', ''),
    '#weight' => $weight++,
  );
  
  return system_settings_form($form);
}

function theme_mywebform_manage_form($form) {
  $output = '';
  $rows = array();
  
  array_unshift($form['#header'], theme('table_select_header_cell'));
  foreach ($form['#rows'] as $key => $row) {
    array_unshift($row, drupal_render($form['checks'][$key]));
    $rows[] = $row;
  }
  
  if (!empty($form['export'])) {
    $output .= drupal_render($form['export']);
  }
  
  $output .= drupal_render($form['options']);
  
  if (empty($rows)) {
    $rows[] = array(
      'data' => array(
        array(
          'data' => t('No results found.'),
          'colspan' => count($form['#header']),
        )
      )
    );
  }
  
  $output .= theme('table', $form['#header'], $rows);
  $output .= drupal_render($form);
  
  return $output;
}

function mywebform_manage_form($form_state) {
  // Execute confirm form for the operation
  if (isset($form_state['values']['operation']) && $form_state['values']['operation'] == 'delete') {
    return mywebform_multiple_delete_confirm($form_state, array_filter($form_state['values']['checks']));
  }
  
  $form = array();
  $header = array(
    array('data' => t('Name'), 'field' => 'name', 'width' => '50%'),
    array('data' => t('Status'), 'field' => 'status'),
    t('Operation'),
    '',
  );
  $rows = array();
  
  $result = db_query("SELECT * FROM {mywebforms} " . tablesort_sql($header));
  while ($data = db_fetch_object($result)) {
    $rows[$data->id] = array(
      l($data->name, 'admin/build/mywebform/' . $data->id),
      $data->status ? t('Enabled') : t('Disabled'),
      l(t('Export fields'), 'admin/build/mywebform/' . $data->id . '/export-fields'),
      l(t('Edit'), 'admin/build/mywebform/' . $data->id . '/edit'),
    );
  }

  foreach ($rows as $id => $row) {
    $checks[$id] = '';
  }
  
  $form['#theme'] = 'mywebform_manage_form';
  $form['#header'] = $header;
  $form['#rows'] = $rows;
  
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  
  $form['options']['operation'] = array(
    '#type' => 'select',
    '#options' => array(
      'disable' => t('Disable'),
      'enable'  => t('Enable'),
      'delete'  => t('Delete'),
    ),
  );
  
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );
  
  $form['checks'] = array(
    '#type' => 'checkboxes',
    '#options' => $checks,
  );
  
  return $form;
}

function mywebform_manage_form_validate($form, &$form_state) {
  if ($form_state['values']['op'] == t('Update')) {
    $checks = array_filter((array) $form_state['values']['checks']);
    if (!count($checks)) {
      form_set_error('', t('No items selected.'));
    }
  }
}

function mywebform_manage_form_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Update')) {
    $ids = array_filter($form_state['values']['checks']);
    
    switch ($form_state['values']['operation']) {
      case 'disable':
        db_query("UPDATE {mywebforms} SET status = 0 WHERE id IN (%s)", implode(', ', $ids));
        break;
        
      case 'enable':
        db_query("UPDATE {mywebforms} SET status = 1 WHERE id IN (%s)", implode(', ', $ids));
        break;
        
      default:
        $form_state['rebuild'] = TRUE;
        return;
    }
    
    drupal_set_message(t('The selected items have been updated'));
  }
}

function mywebform_multiple_delete_confirm($form_state, $ids) {
  $form = array();
  $form['#submit'][] = 'mywebform_multiple_delete_confirm_submit';
  
  $form['checks'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE
  );
  
  $result = db_query("SELECT id, name FROM {mywebforms} WHERE id IN (%s)", implode(', ', $ids));
  while ($data = db_fetch_object($result)) {
    $form['checks'][$data->id] = array(
      '#type' => 'hidden',
      '#value' => $data->id,
      '#prefix' => '<li>',
      '#suffix' => check_plain($data->name) ."</li>\n",
    );
  }
  
  return confirm_form($form, t('Are you sure you want to delete these items?'), $_GET['q']);
}

function mywebform_multiple_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    db_query("DELETE FROM {mywebforms} WHERE id IN (%s)", implode(', ', $form_state['values']['checks']));
    drupal_set_message(t('The selected items have been deleted'));
  }
}

function mywebform_manage_fields_form($form_state, $webform = NULL) {
  // Execute confirm form for the operation
  if (isset($form_state['values']['operation']) && $form_state['values']['operation'] == 'delete') {
    return mywebform_fields_multiple_delete_confirm(
      $form_state,
      array_filter($form_state['values']['checks']));
  }
  
  $form = array();
  $header = array(
    array('data' => t('Name'), 'field' => 'name'),
    array('data' => t('Title'), 'field' => 'title'),
    array('data' => t('Type'), 'field' => 'type'),
    array('data' => t('Widget'), 'field' => 'widget'),
    array('data' => t('Status'), 'field' => 'status'),
    t('Operation'),
  );
  $rows = array();
  
  drupal_set_title(t('Declaration type <em>@name</em>', array('@name' => $webform->name)));
  
  $result = db_query("SELECT * FROM {mywebform_fields} WHERE mywebform_id = %d " . tablesort_sql($header), $webform->id);
  while ($data = db_fetch_object($result)) {
    $rows[$data->id] = array(
      l($data->name, 'admin/build/mywebform/field/' . $data->id . '/view'),
      t($data->title),
      $data->type,
      $data->widget,
      $data->status ? t('Enabled') : t('Disabled'),
      l(t('Edit'), 'admin/build/mywebform/field/' . $data->id . '/edit'),
    );
  }
  
  foreach ($rows as $id => $row) {
    $checks[$id] = '';
  }
  
  $form['#theme'] = 'mywebform_manage_form';
  $form['#header'] = $header;
  $form['#rows'] = $rows;
  
  $form['export'] = array(
    '#type' => 'item',
    '#value' => l(t('Export fields'), 'admin/build/mywebform/' . $webform->id . '/export-fields'),
  );
  
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  
  $form['options']['operation'] = array(
    '#type' => 'select',
    '#options' => array(
      'disable'    => t('Disable'),
      'enable'     => t('Enable'),
      'delete'     => t('Delete'),
    ),
  );
  
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );
  
  $form['checks'] = array(
    '#type' => 'checkboxes',
    '#options' => $checks,
  );
  
  return $form;
}

function mywebform_manage_fields_form_validate($form, &$form_state) {
  if ($form_state['values']['op'] == t('Update')) {
    $checks = array_filter((array) $form_state['values']['checks']);
    if (!count($checks)) {
      form_set_error('', t('No items selected.'));
    }
  }
}

function mywebform_manage_fields_form_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Update')) {
    $ids = array_filter($form_state['values']['checks']);
    
    switch ($form_state['values']['operation']) {
      case 'disable':
        db_query("UPDATE {mywebform_fields} SET status = 0 WHERE id IN (%s)", implode(', ', $ids));
        break;
        
      case 'enable':
        db_query("UPDATE {mywebform_fields} SET status = 1 WHERE id IN (%s)", implode(', ', $ids));
        break;
        
      default:
        $form_state['rebuild'] = TRUE;
        return;
    }
    
    drupal_set_message(t('The selected items have been updated'));
  }
}

function mywebform_fields_multiple_delete_confirm($form_state, $ids) {
  $form = array();
  $form['#submit'][] = 'mywebform_fields_multiple_delete_confirm_submit';
  
  $form['checks'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE
  );
  
  $result = db_query("SELECT id, name FROM {mywebform_fields} WHERE id IN (%s)", implode(', ', $ids));
  while ($data = db_fetch_object($result)) {
    $form['checks'][$data->id] = array(
      '#type' => 'hidden',
      '#value' => $data->id,
      '#prefix' => '<li>',
      '#suffix' => check_plain($data->name) ."</li>\n",
    );
  }
  
  return confirm_form($form, t('Are you sure you want to delete these items?'), $_GET['q']);
}

function mywebform_fields_multiple_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    db_query("DELETE FROM {mywebform_fields} WHERE id IN (%s)", implode(', ', $form_state['values']['checks']));
    drupal_set_message(t('The selected items have been deleted'));
  }
}

function mywebform_field_page($field = NULL) {
  $output = '';
  
  if (empty($field)) {
    drupal_not_found();
    exit();
  }
  
  $field->webform = db_result(db_query("SELECT name FROM {mywebforms} WHERE id = %d", $field->mywebform_id));
  
  $names = array(
    'name'          => 'Name',
    'title'         => 'Title',
    'webform'       => 'Declaration type',
    'weight'        => 'Weight',
    'xpath'         => 'XPath',
    'grid_name'     => 'Grid name',
    'type'          => 'Type',
    'widget'        => 'Widget',
    'disabled'      => 'Disabled',
    'readonly'      => 'Readonly',
    'required'      => 'Required',
    'default_value' => 'Default value',
    'options'       => 'Options',
    'rows'          => 'Rows',
    'check_type'    => 'Check type',
    'class'         => 'Class',
    'minlength'     => 'Minimum length',
    'maxlength'     => 'Maximum length',
    'maxlength_ssemicolon' => 'Maximum length after semicolon',
    'status'        => 'Status',
    'created'       => 'Created',
  );
  
  $rows = array();
  foreach ($names as $key => $title) {
    $rows[] = array('<strong>' . t($title) . '</strong>', htmlspecialchars($field->$key));
  }
  
  $output .= theme('table', array(), $rows);
  
  return $output;
}

function mywebform_export_fields_page($webform) {
  $header = array();
  $rows = array();
  
  foreach ($webform->fields as $field) {
    unset($field->mywebform_id, $field->status, $field->created, $field->aux);
    if (empty($header)) {
      $header = array_keys((array) $field);
    }
    $rows[] = array_values((array) $field);
  }
  
  php_excel_export_table($header, $rows, $webform->tech_name . '.xls');
}