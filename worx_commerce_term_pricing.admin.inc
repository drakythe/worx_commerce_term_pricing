<?php

function worx_commerce_term_pricing_settings_form() {
  $temp_thing = array('BULK01');

  foreach ($temp_thing as $sku) {
    $product = commerce_product_load_by_sku($sku);
    dsm($product);
  }

  $form['contact_information'] = array(
    '#markup' => t('You can leave us a message using the contact form below.'),
  );

  return $form;
}
